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
	//filter->psz_name
	    /* Restore input stream offset (in case previous probed demux failed to
	     * to do so). */
//	    if (vlc_stream_Tell(demux->s) != 0 && vlc_stream_Seek(demux->s, 0))
//	    {
//	        msg_Err(demux, "seek failure before probing");
//	        return VLC_EGENERIC;
//	    }
//
//	    return probe(VLC_OBJECT(demux));
//	}
}



void activate_info_subtitle(vlc_object_t *obj) {
//	 filter_t *sub = vlc_object_create(obj, sizeof(filter_t));

//	 var_Create(VLC_OBJECT(obj)->obj.libvlc, "marq-marquee", VLC_VAR_STRING);

	 msg_Info(obj, "***LOADING MARQUEE");
//	 var_SetChecked( obj->obj.libvlc, "marq-marquee", 0, marquee );

//	 filter_t *spu_filter = calloc(1, sizeof(filter_t));
	 filter_t *spu_filter = vlc_object_create(obj, sizeof(filter_t));
//?	 spu_filter->p_module = module_need( p_sys->p_blend, "video blending", NULL, false );
	 //add a dummy config_chain_t entry
	// spu_filter->p_cfg = calloc(1, sizeof(config_chain_t));

//	 spu_filter->
//	 spu_filter->obj.libvlc = VLC_OBJECT(obj->obj.libvlc);
//	 spu_filter->obj.parent = VLC_OBJECT(obj->obj.parent);
	 /** size_t i_vout_count;
    vout_thread_t **pp_vouts = GetVouts( p_mi, &i_vout_count );
    for( size_t i = 0; i < i_vout_count; ++i )
    {
        var_SetChecked( pp_vouts[i], psz_opt_name, i_type, new_val );
        if( b_sub_source )
            var_TriggerCallback( pp_vouts[i], "sub-source" );
        vlc_object_release( pp_vouts[i] );
    }**/

	 //module_t *sub_module = module_need( sub, "spu", NULL, false );
	 module_t *sub_module = vlc_module_load(obj->obj.libvlc, "sub source", "marq", false, spu_probe, spu_filter);
	 vlc_value_t marquee = { .psz_string = "1.2..3...4.....5...."};

	 var_Set(spu_filter, "marq-marquee", marquee);

	 msg_Info(obj, "***LOADING MARQUEE, ptr is: %p", sub_module);
}



#endif /* MODULES_DEMUX_MMT_MMTP_STATS_MARQUEE__H_ */
