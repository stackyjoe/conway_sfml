#ifndef FFMPEG_WRAPPER_HPP
#define FFMPEG_WRAPPER_HPP

#include <iostream>
#include <numeric>

#include <SFML/Graphics.hpp>

extern "C"{
#include <x264.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavutil/mathematics.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}

template<size_t Height, size_t Width>
class ffmpeg_wrapper
{
using rgba_pixel = std::array<sf::Uint8,4>;
public:
    ffmpeg_wrapper() : codec(nullptr), opt(nullptr), rgbpic(nullptr), yuvpic(nullptr), pkt(), got_output(0) {

        av_register_all(); // Loads the whole database of available codecs and formats.
        av_init_packet(&pkt);
        av_dict_set(&opt, nullptr, nullptr, 0);

        av_log_set_level(AV_LOG_VERBOSE);

        convertCtx= sws_getContext(Width, Height, AV_PIX_FMT_RGB24, Width, Height, AV_PIX_FMT_YUV420P, SWS_BITEXACT, NULL, NULL, NULL); // Preparing to convert my generated RGB images to YUV frames.

        // Preparing the data concerning the format and codec in order to write properly the header, frame data and end of file.
        char const *fmtext="mp4";
        char filename[50];
        sprintf(filename, "GeneratedVideo.%s", fmtext);
        fmt = av_guess_format(fmtext, nullptr, nullptr);
        oc = nullptr;

        if(int return_code = avformat_alloc_output_context2(&oc, NULL, NULL, filename); return_code < 0) {
            std::cout << "avformat_alloc_output_context2 failed" << std::endl;
            std::quick_exit(EXIT_FAILURE);
        }

        stream = avformat_new_stream(oc, nullptr);

        int ret;

        codec = avcodec_find_encoder_by_name("libx264");

        if(codec == nullptr) {
            std::cout << "avcodec_find_encoder_by_name failed" << std::endl;
            std::quick_exit(EXIT_FAILURE);
        }

        // Setting up the codec:
        av_dict_set( &opt, "preset", "slow", 0 );
        av_dict_set( &opt, "crf", "20", 0 );

        if(avcodec_get_context_defaults3(stream->codec, codec) < 0) {
            std::cout << "avcodec_get_context_defaults3 failed" << std::endl;
            std::quick_exit(EXIT_FAILURE);
        }

        c=avcodec_alloc_context3(codec);

        if(c == nullptr) {
            std::cout << "avcodec_alloc_context3 failed" << std::endl;
            std::quick_exit(EXIT_FAILURE);
        }

        c->width = Width;
        c->height = Height;

        c->pix_fmt = AV_PIX_FMT_YUV420P;
        c->color_range = AVCOL_RANGE_JPEG;
        c->codec_type = AVMEDIA_TYPE_VIDEO;
        c->time_base = av_make_q(1,60);

        // Setting up the format, its stream(s), linking with the codec(s) and write the header:
        if (oc->oformat->flags & AVFMT_GLOBALHEADER) // Some formats require a global header.
            c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        if(int return_value = avcodec_open2( c, nullptr, &opt ); return_value < 0) {
            std::cout << "avcodec_open2 failed, ";

            switch(return_value) {
            case AVERROR(EINVAL):
                std::cout << "EINVAL" << std::endl;
                break;
            case AVERROR(ENOMEM):
                std::cout << "ENOMEM" << std::endl;
                break;
            case AVERROR(EAGAIN):
                std::cout << "EAGAIN" << std::endl;
                break;
            default:
                std::cout << "other error" << std::endl;
                break;
            }

            std::quick_exit(EXIT_FAILURE);
        }

        //av_dict_free(&opt);
        stream->time_base=av_make_q(1, 60);
        stream->codec=c; // Once the codec is set up, we need to let the container know which codec are the streams using, in this case the only (video) stream.

        stream->codecpar->codec_id = c->codec_id;
        stream->codecpar->format = AVMEDIA_TYPE_VIDEO;
        stream->codecpar->width = Width;
        stream->codecpar->height = Height;
        stream->codecpar->format = AV_PIX_FMT_YUV420P;

        av_dump_format(oc, 0, filename, 1);
        avio_open(&oc->pb, filename, AVIO_FLAG_WRITE);
        ret=avformat_write_header(oc, &opt);
        av_dict_free(&opt);

        // Allocating memory for each RGB frame, which will be lately converted to YUV:
        rgbpic=av_frame_alloc();
        rgbpic->format=AV_PIX_FMT_RGB24;
        rgbpic->width=Width;
        rgbpic->height=Height;

        ret=av_frame_get_buffer(rgbpic, 0);


        // Allocating memory for each conversion output YUV frame:
        yuvpic=av_frame_alloc();
        yuvpic->format=AV_PIX_FMT_YUV420P;
        yuvpic->width=Width;
        yuvpic->height=Height;

        ret=av_frame_get_buffer(yuvpic, 0);

        if(!avcodec_is_open(c)) {
            std::cout << "avcodec context not open." << std::endl;
            std::quick_exit(EXIT_FAILURE);
        }
    }

    ~ffmpeg_wrapper() {
        size_t i = 0;

        for (got_output = 1; got_output; i++) {
            int ret = avcodec_encode_video2(c, &pkt, NULL, &got_output);


            if (got_output) {
                //fflush(stdout);
                av_packet_rescale_ts(&pkt, av_make_q(1, 60), stream->time_base);
                pkt.stream_index = stream->index;
//                printf("Write frame %6d (size=%6d)\n", i, pkt.size);
                av_interleaved_write_frame(oc, &pkt);
                av_packet_unref(&pkt);
            }
        }


        av_write_trailer(oc); // Writing the end of the file.
        if (!(fmt->flags & AVFMT_NOFILE))
            avio_closep(&(oc->pb)); // Closing the file.

        avcodec_close(stream->codec);
        // Freeing all the allocated memory:
        sws_freeContext(convertCtx);
        av_frame_free(&rgbpic);
        av_frame_free(&yuvpic);
        avformat_free_context(oc);
    }

    void add_frame(std::array<std::array<rgba_pixel, Width>, Height> const *frame_buffer) {
        static size_t i = 0;

        for(size_t y = 0; y < Height; ++y) {
            for(size_t x = 0; x < Width; ++x) {

                rgbpic->data[0][y*rgbpic->linesize[0]+3*x] = (*frame_buffer)[y][x][0];
                rgbpic->data[0][y*rgbpic->linesize[0]+3*x+1] = (*frame_buffer)[y][x][1];
                rgbpic->data[0][y*rgbpic->linesize[0]+3*x+2] = (*frame_buffer)[y][x][2];
            }
        }

        // Not actually scaling anything, but just converting the RGB data to YUV and store it in yuvpic.
        sws_scale(convertCtx, rgbpic->data, rgbpic->linesize, 0, Height, yuvpic->data, yuvpic->linesize);
        av_init_packet(&pkt);
        pkt.data = nullptr;
        pkt.size = 0;

        //rgbpic->pts = i;
        yuvpic->pts = i; // The PTS of the frame are just in a reference unit, unrelated to the format we are using. We set them, for instance, as the corresponding frame number.

        int return_code = avcodec_send_frame( c , yuvpic);
        if ( return_code == 0 ) {
            //std::cout << "Send frame" << std::endl;
            if( avcodec_receive_packet(c,&pkt) == 0) {
                //std::cout << "Receive packet" << std::endl;
                // We set the packet PTS and DTS taking in the account our FPS (second argument) and the time base that our selected format uses (third argument).
                av_packet_rescale_ts(&pkt, av_make_q(1, 60), stream->time_base);
                pkt.stream_index = stream->index;

                //std::cout << "Write frame " << i << "(size=" << pkt.size << " )" << std::endl;
                av_interleaved_write_frame(oc, &pkt); // Write the encoded frame to the mp4 file.
                av_packet_unref(&pkt);
            }
        }
        else {
            switch(return_code) {
            case AVERROR(EINVAL):
                std::cout << "AVERROR(EINVAL)" << std::endl;
                break;
            case AVERROR(EAGAIN):
                std::cout << "AVERROR(EAGAIN)" << std::endl;
                break;
            case AVERROR(ENOMEM):
                std::cout << "AVERROR(ENOMEM)" << std::endl;
                break;
            default:
                std::cout << "AVERROR undetermined" << std::endl;
            }

        }
        ++i;
    }
private:
    AVCodecContext *c;
    AVFormatContext *oc;
    AVOutputFormat * fmt;
    AVStream * stream;
    AVCodec *codec;
    AVDictionary *opt;
    AVFrame *rgbpic;
    AVFrame *yuvpic;
    AVPacket pkt;
    struct SwsContext* convertCtx;
    int got_output;
};

#endif // FFMPEG_WRAPPER_HPP
