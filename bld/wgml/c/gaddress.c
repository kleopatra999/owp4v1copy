/****************************************************************************
*
*                            Open Watcom Project
*
*  Copyright (c) 2004-2008 The Open Watcom Contributors. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  WGML tags :ADDRESS and :eADDRESS  and :ALINE
*
****************************************************************************/
#include    "wgml.h"
#include    "findfile.h"
#include    "gvars.h"

static  bool    first_aline;            // special for first :ALINE



/***************************************************************************/
/*  error msgs for missing or duplicate :ADDRESS :eADDRESS tags            */
/***************************************************************************/

static void g_err_tag( char * tag )
{
    char            tagn[8];

    sprintf_s( tagn, 8, "%c%s", GML_char, tag );
    g_err( ERR_TAG_EXPECTED, tagn );
    file_mac_info();
    err_count++;
    return;
}

static void g_err_tag_prec( char * tag )
{
    char            tagn[8];

    sprintf_s( tagn, 8, "%c%s", GML_char, tag );
    g_err( ERR_TAG_PRECEDING, tagn );
    file_mac_info();
    err_count++;
    return;
}


/***************************************************************************/
/*  :ADDRESS                                                               */
/***************************************************************************/

extern  void    gml_address( const gmltag * entry )
{
    if( ProcFlags.doc_sect != doc_sect_titlep ) {
        g_err( err_tag_wrong_sect, entry->tagname, ":TITLEP section" );
        err_count++;
        show_include_stack();
    }
    if( ProcFlags.address_active ) {    // nested :ADDRESS tag
        g_err_tag( "eADDRESS" );
    }
    ProcFlags.address_active = true;
    first_aline = true;

/*
    pre_top_skip = 0;
    post_top_skip = 0;
    post_skip = 0;
 */

    spacing = layout_work.titlep.spacing;
    scan_start = scan_stop + 1;
    return;
}


/***************************************************************************/
/*  :eADDRESS                                                              */
/***************************************************************************/

extern  void    gml_eaddress( const gmltag * entry )
{
    if( ProcFlags.doc_sect != doc_sect_titlep ) {
        g_err( err_tag_wrong_sect, entry->tagname, ":TITLEP section" );
        err_count++;
        show_include_stack();
    }
    if( !ProcFlags.address_active ) {   // no preceding :ADDRESS tag
        g_err_tag_prec( "ADDRESS" );
    }
    ProcFlags.address_active = false;
    scan_start = scan_stop + 1;
    return;
}

/***************************************************************************/
/*  calc aline position   ( vertical )                                     */
/***************************************************************************/

void    calc_aline_pos( int8_t font, int8_t spacing, bool first )
{

    if( first ) {
        if( !ProcFlags.page_started ) {
            if( bin_driver->y_positive == 0 ) {
                g_cur_v_start = g_page_top
                        - conv_vert_unit( &layout_work.address.pre_skip );
            } else {
                g_cur_v_start = g_page_top
                        + conv_vert_unit( &layout_work.address.pre_skip );
            }
        } else {
            if( bin_driver->y_positive == 0 ) {
                g_cur_v_start -=
                    conv_vert_unit( &layout_work.address.pre_skip );
            } else {
                g_cur_v_start +=
                    conv_vert_unit( &layout_work.address.pre_skip );
            }
        }
    } else {
        if( bin_driver->y_positive == 0 ) {
            g_cur_v_start -= conv_vert_unit( &layout_work.aline.skip );
        } else {
            g_cur_v_start += conv_vert_unit( &layout_work.aline.skip );
        }
    }
    if( bin_driver->y_positive == 0 ) {
        if( g_cur_v_start < g_page_bottom && ProcFlags.page_started ) {
            finish_page();
            document_new_page();
            calc_aline_pos( font, spacing, true );  // 1 recursion
        }
    } else {
        if( g_cur_v_start > g_page_bottom && ProcFlags.page_started ) {
            finish_page();
            document_new_page();
            calc_aline_pos( font, spacing, true );  // 1 recursion
        }
    }
    return;
}

/***************************************************************************/
/*  prepare address line                                                   */
/***************************************************************************/

static void prep_aline( text_line * p_line, char * p )
{
    text_chars  *   curr_t;
    uint32_t        h_left;
    uint32_t        h_right;
    uint32_t        curr_x;

    h_left = g_page_left + conv_hor_unit( &layout_work.address.left_adjust );
    h_right = g_page_right - conv_hor_unit( &layout_work.address.right_adjust );

    if( *p ) {
        curr_t = alloc_text_chars( p, strlen( p ), g_curr_font_num );
    } else {
        curr_t = alloc_text_chars( "aline", 5, g_curr_font_num );
    }
    curr_t->width = cop_text_width( curr_t->text, curr_t->count,
                                    g_curr_font_num );
    while( curr_t->width > (h_right - h_left) ) {   // too long for line
        if( curr_t->count < 2) {        // sanity check
            break;
        }
        curr_t->count -= 1;             // truncate text
        curr_t->width = cop_text_width( curr_t->text, curr_t->count,
                                        g_curr_font_num );
    }
    p_line->first = curr_t;
    curr_x = h_left;
    if( layout_work.address.page_position == pos_center ) {
        if( h_left + curr_t->width < h_right ) {
            curr_x = h_left + (h_right - h_left - curr_t->width) / 2;
        }
    } else if( layout_work.address.page_position == pos_right ) {
        curr_x = h_right - curr_t->width;
    }
    curr_t->x_address = curr_x;
    p_line->ju_x_start = curr_x;

    return;
}
/***************************************************************************/
/*  :ALINE tag                                                             */
/***************************************************************************/

void    gml_aline( const gmltag * entry )
{
    char        *   p;
    text_line       p_line;
    int8_t          font;
    int8_t          spacing;
    int8_t          font_save;
    uint32_t        y_save;

    if( ProcFlags.doc_sect != doc_sect_titlep ) {
        g_err( err_tag_wrong_sect, entry->tagname, ":TITLEP section" );
        err_count++;
        show_include_stack();
    }
    if( !ProcFlags.address_active ) {   // no preceding :ADDRESS tag
        g_err_tag_prec( "ADDRESS" );
    }
    p = scan_start;
    if( *p == '.' ) p++;                // over '.'

    prepare_doc_sect( doc_sect_titlep );// if not already done

    p_line.first = NULL;
    p_line.next  = NULL;
    p_line.line_height = g_max_line_height;

    spacing = layout_work.titlep.spacing;

    font = layout_work.address.font;

    if( font >= wgml_font_cnt ) font = 0;
    font_save = g_curr_font_num;
    g_curr_font_num = font;

    calc_aline_pos( font, spacing, first_aline );
    p_line.y_address = g_cur_v_start;

    prep_aline( &p_line, p );

    ProcFlags.page_started = true;
    y_save = g_cur_v_start;
    process_line_full( &p_line, false );
    g_curr_font_num = font_save;

    if( p_line.first != NULL) {
        add_text_chars_to_pool( &p_line );
        p_line.first = NULL;
    }
    ProcFlags.page_started = true;
    first_aline = false;
    scan_start = scan_stop + 1;
}