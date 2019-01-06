/*
 * mmtp_stats_marquee..h
 *
 *  Created on: Jan 5, 2019
 *      Author: jjustman
 */

#ifndef MODULES_DEMUX_MMT_MMTP_STATS_MARQUEE__H_
#define MODULES_DEMUX_MMT_MMTP_STATS_MARQUEE__H_

#include <vlc_common.h>
#include <vlc_modules.h>
#include <vlc_filter.h>                           /* EnsureUTF8 */


static int spu_probe(void *func, va_list ap)
{
	int (*activate)(vlc_object_t *) = func;
	filter_t *spu_filter = va_arg(ap, filter_t*);

	return activate(VLC_OBJECT(spu_filter));
}

//
//static picture_t *transcode_video_filter_buffer_new(filter_t *p_filter)
//{
//    p_filter->fmt_out.video.i_chroma = p_filter->fmt_out.i_codec;
//    return picture_NewFromFormat(&p_filter->fmt_out.video);
//}
//
//static const struct filter_video_callbacks transcode_filter_video_cbs =
//{
//    .buffer_new = transcode_video_filter_buffer_new,
//};


static es_format_t *__VIDEO_OUTPUT_ES_FORMAT;

filter_t* activate_info_subtitle(vlc_object_t *obj) {

	 msg_Info(obj, "%d:activate_info_subtitle - enter", __LINE__);


	 filter_t *spu_filter = vlc_object_create(obj, sizeof(filter_t));

//?	 spu_filter->p_module = module_need( p_sys->p_blend, "video blending", NULL, false );
	 //add a dummy config_chain_t entry
	// spu_filter->p_cfg = calloc(1, sizeof(config_chain_t));

	 /** size_t i_vout_count;
    vout_thread_t **pp_vouts = GetVouts( p_mi, &i_vout_count );
    for( size_t i = 0; i < i_vout_count; ++i )
    {
        var_SetChecked( pp_vouts[i], psz_opt_name, i_type, new_val );
        if( b_sub_source )
            var_TriggerCallback( pp_vouts[i], "sub-source" );
        vlc_object_release( pp_vouts[i] );
    }**/

	 module_t *sub_module = vlc_module_load(obj->obj.libvlc, "sub source", "marq", false, spu_probe, spu_filter);
	 spu_filter->p_module = sub_module;

	 vlc_value_t marquee = { .psz_string = "1.2..3...4.....5...."};
	 var_Set(spu_filter, "marq-marquee", marquee);

	 msg_Info(obj, "***LOADING MARQUEE, done: ptr is: %p", sub_module);

	 filter_chain_t *filter_chain = filter_chain_NewVideo(obj, false, NULL);
	 filter_chain_AppendFilter(filter_chain, "spu", NULL, NULL, __VIDEO_OUTPUT_ES_FORMAT);

	 return spu_filter;
}



#endif /* MODULES_DEMUX_MMT_MMTP_STATS_MARQUEE__H_ */
