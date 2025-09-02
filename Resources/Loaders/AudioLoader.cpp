#include "AudioLoader.h"
#include "../AssetManager.h"
#include "../../Utils/Logger.h"


extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
}

namespace
{
    struct AVIOBuffer
    {
        AVIOContext* ctx = nullptr;
        const uint8_t* data = nullptr;
        size_t size = 0;
        size_t pos = 0;

        ~AVIOBuffer()
        {
            if (ctx)
            {
                av_freep(&ctx->buffer);
                avio_context_free(&ctx);
            }
        }
    };

    int ReadPacket(void* opaque, uint8_t* buf, int buf_size)
    {
        auto* b = reinterpret_cast<AVIOBuffer*>(opaque);
        size_t remaining = b->size - b->pos;
        int toCopy = static_cast<int>(std::min(remaining, static_cast<size_t>(buf_size)));
        if (toCopy <= 0) return AVERROR_EOF;
        memcpy(buf, b->data + b->pos, toCopy);
        b->pos += toCopy;
        return toCopy;
    }
}

sk_sp<RuntimeAudio> AudioLoader::LoadAsset(const AssetMetadata& metadata)
{
    return DecodeToPCM(metadata);
}

sk_sp<RuntimeAudio> AudioLoader::LoadAsset(const Guid& guid)
{
    const AssetMetadata* meta = AssetManager::GetInstance().GetMetadata(guid);
    if (!meta || meta->type != AssetType::Audio) return nullptr;
    return DecodeToPCM(*meta);
}

sk_sp<RuntimeAudio> AudioLoader::DecodeToPCM(const AssetMetadata& meta) const
{
    if (!meta.importerSettings["encodedData"])
    {
        LogError("AudioLoader: No encodedData for asset {}", meta.assetPath.string());
        return nullptr;
    }

    YAML::Binary bin = meta.importerSettings["encodedData"].as<YAML::Binary>();
    AVFormatContext* fmt = avformat_alloc_context();
    if (!fmt) return nullptr;

    AVIOBuffer io;
    io.size = bin.size();
    io.data = bin.data();
    unsigned char* buffer = static_cast<unsigned char*>(av_malloc(4096));
    io.ctx = avio_alloc_context(buffer, 4096, 0, &io, &ReadPacket, nullptr, nullptr);
    fmt->pb = io.ctx;

    if (avformat_open_input(&fmt, nullptr, nullptr, nullptr) < 0)
    {
        avformat_free_context(fmt);
        return nullptr;
    }
    if (avformat_find_stream_info(fmt, nullptr) < 0)
    {
        avformat_close_input(&fmt);
        return nullptr;
    }

    int audioStreamIndex = av_find_best_stream(fmt, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audioStreamIndex < 0)
    {
        avformat_close_input(&fmt);
        return nullptr;
    }

    AVStream* stream = fmt->streams[audioStreamIndex];
    AVCodecParameters* par = stream->codecpar;
    const AVCodec* codec = avcodec_find_decoder(par->codec_id);
    if (!codec)
    {
        avformat_close_input(&fmt);
        return nullptr;
    }

    AVCodecContext* dec = avcodec_alloc_context3(codec);
    if (!dec)
    {
        avformat_close_input(&fmt);
        return nullptr;
    }
    if (avcodec_parameters_to_context(dec, par) < 0 ||
        avcodec_open2(dec, codec, nullptr) < 0)
    {
        avcodec_free_context(&dec);
        avformat_close_input(&fmt);
        return nullptr;
    }


    AVChannelLayout outLayout;
    if (targetChannels == 1)
    {
        outLayout = AV_CHANNEL_LAYOUT_MONO;
    }
    else
    {
        outLayout = AV_CHANNEL_LAYOUT_STEREO;
    }
    SwrContext* swr = nullptr;
    int ret = swr_alloc_set_opts2(
        &swr,
        &outLayout,
        AV_SAMPLE_FMT_FLT,
        targetSampleRate,
        &dec->ch_layout,
        dec->sample_fmt,
        dec->sample_rate,
        0, nullptr);

    if (ret < 0 || swr == nullptr)
    {
        char errBuf[AV_ERROR_MAX_STRING_SIZE] = {0};
        std::string err = av_make_error_string(errBuf, AV_ERROR_MAX_STRING_SIZE, ret);
        LogError("AudioLoader: Failed to allocate SwrContext for asset {}: {}", meta.assetPath.string(), err);
        if (swr) swr_free(&swr);
        avcodec_free_context(&dec);
        avformat_close_input(&fmt);
        return nullptr;
    }
    if (!swr || swr_init(swr) < 0)
    {
        if (swr) swr_free(&swr);
        avcodec_free_context(&dec);
        avformat_close_input(&fmt);
        return nullptr;
    }

    std::vector<float> outPCM;
    outPCM.reserve(1024 * 1024);

    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    while (av_read_frame(fmt, pkt) >= 0)
    {
        if (pkt->stream_index != audioStreamIndex)
        {
            av_packet_unref(pkt);
            continue;
        }

        if (avcodec_send_packet(dec, pkt) == 0)
        {
            while (avcodec_receive_frame(dec, frame) == 0)
            {
                int outSamples = swr_get_out_samples(swr, frame->nb_samples);
                int outChannels = targetChannels > 0 ? targetChannels : 2;

                std::vector<float> tmp(static_cast<size_t>(outSamples) * outChannels);
                uint8_t* outData[1] = {reinterpret_cast<uint8_t*>(tmp.data())};
                int converted = swr_convert(swr, outData, outSamples,
                                            (const uint8_t**)frame->extended_data, frame->nb_samples);
                if (converted > 0)
                {
                    size_t toAppend = static_cast<size_t>(converted) * outChannels;
                    outPCM.insert(outPCM.end(), tmp.data(), tmp.data() + toAppend);
                }
            }
        }
        av_packet_unref(pkt);
    }


    avcodec_send_packet(dec, nullptr);
    while (avcodec_receive_frame(dec, frame) == 0)
    {
        int outSamples = swr_get_out_samples(swr, frame->nb_samples);
        int outChannels = targetChannels > 0 ? targetChannels : 2;
        std::vector<float> tmp(static_cast<size_t>(outSamples) * outChannels);
        uint8_t* outData[1] = {reinterpret_cast<uint8_t*>(tmp.data())};
        int converted = swr_convert(swr, outData, outSamples,
                                    (const uint8_t**)frame->extended_data, frame->nb_samples);
        if (converted > 0)
        {
            size_t toAppend = static_cast<size_t>(converted) * outChannels;
            outPCM.insert(outPCM.end(), tmp.data(), tmp.data() + toAppend);
        }
    }

    av_frame_free(&frame);
    av_packet_free(&pkt);
    swr_free(&swr);
    avcodec_free_context(&dec);
    avformat_close_input(&fmt);

    if (outPCM.empty()) return nullptr;

    auto ra = sk_make_sp<RuntimeAudio>();
    ra->SetPCMData(std::move(outPCM), targetSampleRate, targetChannels > 0 ? targetChannels : 2);
    return ra;
}
