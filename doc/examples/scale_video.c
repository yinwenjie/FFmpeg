/*
 * Copyright (c) 2012 Stefano Sabatini
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @file libswscale API usage example
 * @example scale_video.c
 *
 * Generate a synthetic video signal and use libswscale to perform rescaling.
 */

#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include <libavutil/parseutils.h>
#include <libswscale/swscale.h>

static void fill_yuv_image(uint8_t *data[4], int linesize[4],
                           int width, int height, int frame_index)
{
    int x, y;

    /* Y */
    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
            data[0][y * linesize[0] + x] = x + y + frame_index * 3;

    /* Cb and Cr */
    for (y = 0; y < height / 2; y++) {
        for (x = 0; x < width / 2; x++) {
            data[1][y * linesize[1] + x] = 128 + y + frame_index * 2;
            data[2][y * linesize[2] + x] = 64 + x + frame_index * 5;
        }
    }
}

int main(int argc, char **argv)
{
    AVFrame *src_frame = NULL, *dst_frame = NULL;
    int src_w = 320, src_h = 240, dst_w, dst_h;
    enum AVPixelFormat src_pix_fmt = AV_PIX_FMT_YUV420P, dst_pix_fmt = AV_PIX_FMT_RGB24;
    const char *dst_size = NULL;
    const char *dst_filename = NULL;
    FILE *dst_file;
    int dst_linesize;
    SwsContext *sws_ctx = NULL;
    int i, y, ret;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s output_file output_size\n"
                "API example program to show how to scale an image with libswscale.\n"
                "This program generates a series of pictures, rescales them to the given "
                "output_size and saves them to an output file named output_file.\n"
                "\n", argv[0]);
        exit(1);
    }
    dst_filename = argv[1];
    dst_size     = argv[2];

    if (av_parse_video_size(&dst_w, &dst_h, dst_size) < 0) {
        fprintf(stderr,
                "Invalid size '%s', must be in the form WxH or a valid size abbreviation\n",
                dst_size);
        exit(1);
    }

    dst_file = fopen(dst_filename, "wb");
    if (!dst_file) {
        fprintf(stderr, "Could not open destination file %s\n", dst_filename);
        exit(1);
    }

    /* create scaling context; the properties of the conversion are taken
     * from the frames passed to sws_scale_frame() */
    sws_ctx = sws_alloc_context();
    if (!sws_ctx) {
        fprintf(stderr, "Could not allocate the scaling context\n");
        ret = AVERROR(ENOMEM);
        goto end;
    }
    sws_ctx->scaler = SWS_SCALE_BILINEAR;

    /* allocate source and destination frames */
    src_frame = av_frame_alloc();
    dst_frame = av_frame_alloc();
    if (!src_frame || !dst_frame) {
        fprintf(stderr, "Could not allocate frames\n");
        ret = AVERROR(ENOMEM);
        goto end;
    }

    src_frame->format = src_pix_fmt;
    src_frame->width  = src_w;
    src_frame->height = src_h;
    if ((ret = av_frame_get_buffer(src_frame, 0)) < 0) {
        fprintf(stderr, "Could not allocate source image\n");
        goto end;
    }

    dst_frame->format = dst_pix_fmt;
    dst_frame->width  = dst_w;
    dst_frame->height = dst_h;
    if ((ret = av_frame_get_buffer(dst_frame, 0)) < 0) {
        fprintf(stderr, "Could not allocate destination image\n");
        goto end;
    }

    /* number of bytes of one line of the destination image without padding */
    dst_linesize = av_image_get_linesize(dst_pix_fmt, dst_w, 0);

    for (i = 0; i < 100; i++) {
        /* generate synthetic video */
        fill_yuv_image(src_frame->data, src_frame->linesize, src_w, src_h, i);

        /* convert to destination format */
        ret = sws_scale_frame(sws_ctx, dst_frame, src_frame);
        if (ret < 0) {
            fprintf(stderr, "Error while scaling: %s\n", av_err2str(ret));
            goto end;
        }

        /* write scaled image to file; the lines of the frame may be padded,
         * the rawvideo file must not contain the padding */
        for (y = 0; y < dst_h; y++) {
            if (fwrite(dst_frame->data[0] + y * dst_frame->linesize[0],
                       1, dst_linesize, dst_file) != dst_linesize) {
                fprintf(stderr, "Error writing to %s\n", dst_filename);
                ret = AVERROR(EIO);
                goto end;
            }
        }
    }

    if (fflush(dst_file)) {
        fprintf(stderr, "Error writing to %s\n", dst_filename);
        ret = AVERROR(EIO);
        goto end;
    }

    fprintf(stderr, "Scaling succeeded. Play the output file with the command:\n"
           "ffplay -f rawvideo -pix_fmt %s -video_size %dx%d %s\n",
           av_get_pix_fmt_name(dst_pix_fmt), dst_w, dst_h, dst_filename);

end:
    fclose(dst_file);
    av_frame_free(&src_frame);
    av_frame_free(&dst_frame);
    sws_free_context(&sws_ctx);
    return ret < 0;
}
