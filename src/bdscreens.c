#define inline _inline
#include <stdio.h>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

const int32_t ff_yuv2rgb_coeffs[4] =  {117504, 138453, 13954, 34903};

int main(int argc, char **argv)
{
    AVFormatContext *format;
    AVCodecContext *decoder_ctx;
    AVCodec *decoder;
    AVCodecContext *encoder_ctx;
    AVCodec *encoder;
    int stream_idx;
    int video_stream_idx = -1;
    int width, height;
    AVFrame *frame_420;
    AVFrame *frame_444;
    AVFrame *frame_RGB;
    AVPacket packet;
    static struct SwsContext *convert_444 = 0;
    static struct SwsContext *convert_RGB = 0;
    uint8_t *in_buffer_420;
    uint8_t *out_buffer_420;
    int bytes_420;
    uint8_t *in_buffer_444;
    uint8_t *out_buffer_444;
    int bytes_444;
    uint8_t *in_buffer_RGB;
    uint8_t *out_buffer_RGB;
    int bytes_RGB;
    int size_RGB;
    int is_frame_finished;
    int frame_idx = 0;
    char png_name[128];
    FILE *png_file;

    av_register_all();

    if (argc < 2)
    {
        fprintf(stderr, "Please specify an input .M2TS\n");
        return -1;
    }
    if (av_open_input_file(&format, argv[1], NULL, 0, NULL) != 0)
    {
        fprintf(stderr, "Couldn't open input file!\n");
        return -1;
    }
    if (av_find_stream_info(format) < 0)
    {
        fprintf(stderr, "Couldn't find stream info!\n");
        return -1;
    }
    
    dump_format(format, 0, argv[1], 0);

    for (stream_idx = 0; stream_idx < (int)format->nb_streams; stream_idx++)
    {
        if (format->streams[stream_idx]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_stream_idx = stream_idx;
        }
        if (format->streams[stream_idx]->id == 4113)
        {
            break;
        }
    }
    if (video_stream_idx == -1)
    {
        fprintf(stderr, "No primary video stream found!\n");
        return -1;
    }

    decoder_ctx = format->streams[video_stream_idx]->codec;
    decoder = avcodec_find_decoder(decoder_ctx->codec_id);
    if (decoder == 0) 
    {
        fprintf(stderr, "Unsupported codec!\n");
        return -1;
    }
    if (avcodec_open(decoder_ctx, decoder) < 0)
    {
        fprintf(stderr, "Couldn't open the decoder!\n");
        return -1;
    }

    width = decoder_ctx->width;
    height = decoder_ctx->height;

    encoder_ctx = avcodec_alloc_context();
    encoder = avcodec_find_encoder(CODEC_ID_PNG);
    if (encoder == 0) 
    {
        fprintf(stderr, "Couldn't find the PNG encoder!\n");
        return -1;
    }
    if (avcodec_open(encoder_ctx, encoder) < 0)
    {
        fprintf(stderr, "Couldn't open the PNG encoder!\n");
        return -1;
    }
    encoder_ctx->compression_level = 10;
    encoder_ctx->width = width;
    encoder_ctx->height = height;
    encoder_ctx->pix_fmt = PIX_FMT_RGB32;

    if (convert_444 == NULL) 
    {
        convert_444 = sws_getContext(
            width, 
            height, 
            decoder_ctx->pix_fmt, 
            width, 
            height, 
            PIX_FMT_YUV444P,
            SWS_FAST_BILINEAR | SWS_BITEXACT | SWS_ACCURATE_RND | SWS_FULL_CHR_H_INT | SWS_FULL_CHR_H_INP, 
            NULL, NULL, NULL);

        if (convert_444 == NULL) 
        {
            fprintf(stderr, "Couldn't initialize the YUV 4:4:4 conversion context!\n");
            return -1;
        }
    }

    if (convert_RGB == NULL) 
    {
        int w = decoder_ctx->width;
        int h = decoder_ctx->height;
        convert_RGB = sws_getContext(
            width, 
            height, 
            PIX_FMT_YUV444P, 
            width, 
            height, 
            PIX_FMT_RGB32, 
            SWS_FAST_BILINEAR | SWS_BITEXACT | SWS_ACCURATE_RND | SWS_FULL_CHR_H_INT | SWS_FULL_CHR_H_INP, 
            NULL, NULL, NULL);

        if (convert_RGB == NULL) 
        {
            fprintf(stderr, "Couldn't initialize the RGB conversion context!\n");
            return -1;
        }
    }
    
    sws_setColorspaceDetails(convert_RGB, ff_yuv2rgb_coeffs, 0, ff_yuv2rgb_coeffs, 1, 0, 1<<16, 1<<16);

    frame_420 = avcodec_alloc_frame();
    frame_444 = avcodec_alloc_frame();
    frame_RGB = avcodec_alloc_frame();

    bytes_420 = avpicture_get_size(PIX_FMT_YUV420P, width, height);
    in_buffer_420 = (uint8_t *)av_malloc(bytes_420 * sizeof(uint8_t));
    out_buffer_420 = (uint8_t *)av_malloc(bytes_420 * sizeof(uint8_t));
    avpicture_fill((AVPicture *)frame_420, in_buffer_420, PIX_FMT_YUV420P, width, height);

    bytes_444 = avpicture_get_size(PIX_FMT_YUV444P, width, height);
    in_buffer_444 = (uint8_t *)av_malloc(bytes_444 * sizeof(uint8_t));
    out_buffer_444 = (uint8_t *)av_malloc(bytes_444 * sizeof(uint8_t));
    avpicture_fill((AVPicture *)frame_444, in_buffer_444, PIX_FMT_YUV444P, width, height);

    bytes_RGB = avpicture_get_size(PIX_FMT_RGB32, width, height);
    in_buffer_RGB = (uint8_t *)av_malloc(bytes_RGB * sizeof(uint8_t));
    out_buffer_RGB = (uint8_t *)av_malloc(bytes_RGB * sizeof(uint8_t));
    avpicture_fill((AVPicture *)frame_RGB, in_buffer_RGB, PIX_FMT_RGB32, width, height);

    while (av_read_frame(format, &packet) >= 0) 
    {
        if (packet.stream_index == video_stream_idx) 
        {
            avcodec_decode_video2(decoder_ctx, frame_420, &is_frame_finished, &packet);

            if (is_frame_finished) 
            {
                char pict_type = av_get_pict_type_char(frame_420->pict_type);
                ++frame_idx;

                if (pict_type == 'I')
                {
                    sprintf(png_name, "%07d-%c.png", decoder_ctx->frame_number, pict_type);
                    png_file = fopen(png_name, "wb");
                    if (png_file == 0) continue;
                            
                    sws_scale(
                        convert_444, 
                        frame_420->data, 
                        frame_420->linesize, 
                        0, 
                        height, 
                        frame_444->data, 
                        frame_444->linesize);

                    sws_scale(
                        convert_RGB, 
                        frame_444->data, 
                        frame_444->linesize, 
                        0, 
                        height, 
                        frame_RGB->data, 
                        frame_RGB->linesize);

                    size_RGB = avcodec_encode_video(encoder_ctx, out_buffer_RGB, bytes_RGB, frame_RGB);

                    fwrite(out_buffer_RGB, sizeof(uint8_t), size_RGB, png_file);
                    fclose(png_file);
                }
            }
            av_free_packet(&packet);
        }
    }

    av_free(in_buffer_420);
    av_free(out_buffer_420);
    av_free(frame_420);

    av_free(in_buffer_444);
    av_free(out_buffer_444);
    av_free(frame_444);

    av_free(in_buffer_RGB);
    av_free(out_buffer_RGB);
    av_free(frame_RGB);

    sws_freeContext(convert_444);
    sws_freeContext(convert_RGB);

    av_close_input_file(format);

    return 0;
}

