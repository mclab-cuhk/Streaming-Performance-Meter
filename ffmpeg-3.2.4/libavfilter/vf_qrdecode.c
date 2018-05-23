/*
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * copy video filter
 */

#include <inttypes.h>

#include "libavutil/adler32.h"
#include "libavutil/display.h"
#include "libavutil/imgutils.h"
#include "libavutil/internal.h"
#include "libavutil/pixdesc.h"
#include "libavutil/stereo3d.h"
#include "libavutil/timestamp.h"

#include "libavutil/opt.h"
#include "drawutils.h"
#include "formats.h"

#include "avfilter.h"
#include "internal.h"
#include "video.h"

#include "quirc_internal.h"

#include <zbar.h>

zbar_decoder_t *decoder;

static void symbol_handler (zbar_decoder_t *decoder)
{
    zbar_symbol_type_t sym = zbar_decoder_get_type(decoder);
    if(sym <= ZBAR_PARTIAL)
        return;
    printf("%s%s:%s\n",
           zbar_get_symbol_name(sym),
           zbar_get_addon_name(sym),
           zbar_decoder_get_data(decoder));
    /* FIXME add check! */
}


static int query_formats(AVFilterContext *ctx)
{
    AVFilterFormats *formats = NULL;
    int fmt;

    for (fmt = 0; av_pix_fmt_desc_get(fmt); fmt++) {
        const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(fmt);
        int ret;
        if (desc->flags & AV_PIX_FMT_FLAG_HWACCEL)
            continue;
        if ((ret = ff_add_format(&formats, fmt)) < 0)
            return ret;
    }

    return ff_set_common_formats(ctx, formats);
}

struct quirc *q = NULL;

/*
static void draw_blank_frame(AVFilterContext *ctx, AVFrame *out_buf)
{
    TileContext *tile    = ctx->priv;
    AVFilterLink *inlink = ctx->inputs[0];
    unsigned x0, y0;

    get_current_tile_pos(ctx, &x0, &y0);
    ff_fill_rectangle(&tile->draw, &tile->blank, out_buf->data, out_buf->linesize, x0, y0, inlink->w, inlink->h);
    tile->current++;
}
*/

static int filter_frame(AVFilterLink *inlink, AVFrame *in)
{
    AVFilterContext *ctx = inlink->dst;
    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(inlink->format);

    AVFilterLink *outlink = inlink->dst->outputs[0];
    AVFrame *out = ff_get_video_buffer(outlink, in->width, in->height);

    if (!out) {
        av_frame_free(&in);
        return AVERROR(ENOMEM);
    }
    av_frame_copy_props(out, in);
    av_frame_copy(out, in);

//RUDOLF-barcode
	if (1)
	{
		int i, j, nbarcodes = 0;
        uint8_t *raw_image = NULL;

		zbar_image_t *img = zbar_image_create();
		zbar_image_scanner_t *scanner = zbar_image_scanner_create();
		const zbar_symbol_t *symbol;

		const uint8_t roffset = 0;//s->rgba_map[R];
		const uint8_t goffset = 1;//s->rgba_map[G];
		const uint8_t boffset = 2;//s->rgba_map[B];
		const uint8_t *srcrow = in->data[0];
		uint8_t *dstrow = NULL;

		if ((raw_image = (uint8_t*)malloc(in->width * in->height)) != NULL)
		{
			dstrow = raw_image;
			for(i = 0; i < in->height; i++)
			{
				const uint8_t *src = srcrow;
				uint8_t *dst = dstrow;

				for(j = 0; j < in->width; j++)
				{
					const uint8_t rin = src[j*3 + roffset];
					const uint8_t gin = src[j*3 + goffset];
					const uint8_t bin = src[j*3 + boffset];

					if ((int)rin > 50 || (int)gin > 50 || (int)bin > 50)
						dst[j] = 255;
					else
						dst[j] = 0;
				}

				srcrow += in->linesize[0];
				dstrow += in->width;
			}

			zbar_image_set_format(img, *(int*)"Y800");
			zbar_image_set_size(img, in->width, in->height);
			zbar_image_set_data(img, raw_image, in->width * in->height, zbar_image_free_data);

			zbar_image_scanner_set_config(scanner, 0, ZBAR_CFG_ENABLE, 1);
			nbarcodes = zbar_scan_image(scanner, img);

			i = 0;
			symbol = zbar_image_first_symbol(img);
			for(; symbol; symbol = zbar_symbol_next(symbol))
			{
				zbar_symbol_type_t typ = zbar_symbol_get_type(symbol);
				const char *data = zbar_symbol_get_data(symbol);
				fprintf(stdout, "frame %06d-%d BARCODE decoded %s symbol \"%s\"\n", inlink->frame_count, i++, zbar_get_symbol_name(typ), data);
				fflush(stdout);
			}

//			free(raw_image);
		}

		zbar_scanner_destroy(scanner);
		zbar_image_destroy(img);
	}

//    av_log(ctx, AV_LOG_INFO, "pts_time:%-7s video in fmt: %s\n", av_ts2timestr(in->pts, &inlink->time_base), desc->name);
//RUDOLF-qrcode
    if (0)
    {
        struct quirc_code code;
        struct quirc_data data;
        quirc_decode_error_t err;
        uint8_t *raw_image = NULL;

        int i, j, qrcnt;
        char *qrdata = NULL;

        const uint8_t roffset = 0;//s->rgba_map[R];
        const uint8_t goffset = 1;//s->rgba_map[G];
        const uint8_t boffset = 2;//s->rgba_map[B];
        const uint8_t *srcrow = in->data[0];
        uint8_t *dstrow = NULL;
        
        q = quirc_new();
        quirc_resize(q, in->width, in->height);
        raw_image = quirc_begin(q, NULL, NULL);
        // copy raw frame data into q image
        dstrow = raw_image;
        for(i = 0; i < in->height; i++)
        {
            const uint8_t *src = srcrow;
            uint8_t *dst = dstrow;
            
            for(j = 0; j < in->width; j++)
            {
                const uint8_t rin = src[j*3 + roffset];
                const uint8_t gin = src[j*3 + goffset];
                const uint8_t bin = src[j*3 + boffset];

				if ((int)rin > 100 || (int)gin > 100 || (int)bin > 100)
					dst[j] = 255;
				else
					dst[j] = 0;
//                dst[j] = ((int)rin + (int)gin + (int)bin)/3;

//				if ((i < 20 || i > 330) && (j < 320 || j > 640))
//					dst[j] = 255;
            }
            
            srcrow += in->linesize[0];
            dstrow += in->width;
        }
        quirc_end(q);
        
        qrdata = "";
		qrcnt = quirc_count(q);
        if (qrcnt > 0)
        {
            quirc_extract(q, 0, &code);		// always take the last one
            err = quirc_decode(&code, &data);
            if (strlen(data.payload) > 10)
            {
                char *sp = data.payload;
                while (*sp)
                {
                    if (*sp == ' ') *sp = ',';
                    sp++;
                }
                qrdata = data.payload;
            }
        }
        av_log(ctx, AV_LOG_INFO, "\nn,%5"PRId64",pts,%-7s,QRCODE,%d,%s\n", inlink->frame_count, av_ts2timestr(in->pts, &inlink->time_base), qrcnt, qrdata);
        if (qrcnt > 1)
        {
            quirc_extract(q, 1, &code);		// always take the last one
            err = quirc_decode(&code, &data);
            if (strlen(data.payload) > 10)
            {
                char *sp = data.payload;
                while (*sp)
                {
                    if (*sp == ' ') *sp = ',';
                    sp++;
                }
                qrdata = data.payload;
            }
        }
        av_log(ctx, AV_LOG_INFO, "\nn,%5"PRId64",pts,%-7s,QRCODE,%d,%s\n", inlink->frame_count, av_ts2timestr(in->pts, &inlink->time_base), qrcnt, qrdata);
    }
    
    av_frame_free(&in);
    return ff_filter_frame(outlink, out);
}

static const AVFilterPad avfilter_vf_qrdecode_inputs[] = {
    {
        .name         = "default",
        .type         = AVMEDIA_TYPE_VIDEO,
        .filter_frame = filter_frame,
    },
    { NULL }
};

static const AVFilterPad avfilter_vf_qrdecode_outputs[] = {
    {
        .name = "default",
        .type = AVMEDIA_TYPE_VIDEO,
    },
    { NULL }
};

AVFilter ff_vf_qrdecode = {
    .name        = "qrdecode",
    .description = NULL_IF_CONFIG_SMALL("QR decode the frame data to text file"),
    .inputs      = avfilter_vf_qrdecode_inputs,
    .outputs     = avfilter_vf_qrdecode_outputs,
    .query_formats = query_formats,
};
