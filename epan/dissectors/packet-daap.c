/* packet-daap.c
 * Routines for Digital Audio Access Protocol dissection
 * Copyright 2004, Kelly Byrd <kbyrd@memcpy.com>
 *
 * $Id$
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>
#include <epan/packet.h>
#include <epan/dissectors/packet-http.h>

#define TCP_PORT_DAAP 3689

/* DAAP tags */
/* Some information taken from http://tapjam.net/daap/ */
/* and http://www.deleet.de/projekte/daap/?ContentCodes */
/* Container tags */
#define daap_mcon       0x6d636f6e
#define daap_msrv       0x6d737276
#define daap_mccr       0x6d636372
#define daap_mdcl       0x6d64636c
#define daap_mlog       0x6d6c6f67
#define daap_mupd       0x6d757064
#define daap_avdb       0x61766462
#define daap_mlcl       0x6d6c636c
#define daap_mlit       0x6d6c6974
#define daap_mbcl       0x6d62636c
#define daap_adbs       0x61646273
#define daap_aply       0x61706c79
#define daap_apso       0x6170736f
#define daap_mudl       0x6d75646c
#define daap_abro       0x6162726f
#define daap_abar	0x61626172
#define daap_arsv	0x61727376
#define daap_abal       0x6162616c
#define daap_abcp       0x61626370
#define daap_abgn       0x6162676e
#define daap_prsv       0x70727376
#define daap_arif       0x61726966
#define daap_mctc	0x6d637463
/* String tags */
#define daap_minm       0x6d696e6d
#define daap_msts       0x6d737473
#define daap_mcnm       0x6d636e6d
#define daap_mcna       0x6d636e61
#define daap_asal       0x6173616c
#define daap_asar       0x61736172
#define daap_ascm       0x6173636d
#define daap_asfm       0x6173666d
#define daap_aseq       0x61736571
#define daap_asgn       0x6173676e
#define daap_asdt       0x61736474
#define daap_asul       0x6173756c
#define daap_ascp       0x61736370
#define daap_asct	0x61736374
#define daap_ascn	0x6173636e
#define daap_aslc	0x61736c63
#define daap_asky	0x61736b79
#define daap_aeSN	0x6165534e
#define daap_aeNN	0x61654e4e
#define daap_aeEN	0x6165454e
#define daap_assn	0x6173736e
#define daap_assa	0x61737361
#define daap_assl	0x6173736c
#define daap_assc	0x61737363
#define daap_asss	0x61737373
#define daap_asaa	0x61736161
#define daap_aspu	0x61737075
#define daap_aeCR	0x61654352
/* uint64 tags */
#define daap_mper       0x6d706572
#define daap_aeGU	0x61654755
#define daap_aeGR	0x61654752
#define daap_asai	0x61736169
#define daap_asls	0x61736c73
/* uint32 tags */
#define daap_mstt       0x6d737474
#define daap_musr       0x6d757372
#define daap_miid       0x6d696964
#define daap_mcti       0x6d637469
#define daap_mpco       0x6d70636f
#define daap_mimc       0x6d696d63
#define daap_mrco       0x6d72636f
#define daap_mtco       0x6d74636f
#define daap_mstm       0x6d73746d
#define daap_msdc       0x6d736463
#define daap_mlid       0x6d6c6964
#define daap_msur       0x6d737572
#define daap_asda       0x61736461
#define daap_asdm       0x6173646d
#define daap_assr       0x61737372
#define daap_assz       0x6173737a
#define daap_asst       0x61737374
#define daap_assp       0x61737370
#define daap_astm       0x6173746d
#define daap_aeNV       0x61654e56
#define daap_ascd	0x61736364
#define daap_ascs	0x61736373
#define daap_aeSV	0x61655356
#define daap_aePI	0x61655049
#define daap_aeCI	0x61654349
#define daap_aeGI	0x61654749
#define daap_aeAI	0x61654149
#define daap_aeSI	0x61655349
#define daap_aeES	0x61654553
#define daap_aeSU	0x61655355
#define daap_asbo	0x6173626f
#define daap_aeGH	0x61654748
#define daap_aeGD	0x61654744
#define daap_aeGE	0x61654745
#define daap_meds	0x6d656473
/*TODO:
#define daap_msto		0x6d7374OO utcoffset
*/
/* uint16 tags */
#define daap_mcty       0x6d637479
#define daap_asbt       0x61736274
#define daap_asbr       0x61736272
#define daap_asdc       0x61736463
#define daap_asdn       0x6173646e
#define daap_astc       0x61737463
#define daap_astn       0x6173746e
#define daap_asyr       0x61737972
#define daap_ased	0x61736564
/* byte  tags */
#define daap_mikd       0x6d696b64
#define daap_msau       0x6d736175
#define daap_msty       0x6d737479
#define daap_asrv       0x61737276 /* XXX: may be uint16 in newer iTunes versions! */
#define daap_asur       0x61737572
#define daap_asdk       0x6173646b
#define daap_muty	0x6d757479
#define daap_msas	0x6d736173
#define daap_aeHV	0x61654856
#define daap_aeHD	0x61654844
#define daap_aePC	0x61655043
#define daap_aePP	0x61655050
#define daap_aeMK	0x61654d4b
#define daap_aeSG	0x61655347
#define daap_apsm	0x6170736d
#define daap_aprm	0x6170726d
#define daap_asgp	0x61736770
#define daap_aePS	0x61655053
#define daap_asbk	0x6173626b
/* boolean  tags */
#define daap_mslr       0x6d736c72
#define daap_msal       0x6d73616c
#define daap_msup       0x6d737570
#define daap_mspi       0x6d737069
#define daap_msex       0x6d736578
#define daap_msbr       0x6d736272
#define daap_msqy       0x6d737179
#define daap_msix       0x6d736978
#define daap_msrs       0x6d737273
#define daap_asco       0x6173636f
#define daap_asdb       0x61736462
#define daap_abpl       0x6162706c
#define daap_aeSP       0x61655350
#define daap_ashp	0x61736870
/* version (32-bit)*/
#define daap_mpro       0x6d70726f
#define daap_apro       0x6170726f
/* date/time */
/* TODO:
#define daap_mstc 0xMMSSTTCC utctime
#define daap_asdr ("daap.songdatereleased")
#define daap_asdp ("daap.songdatepurchased")
*/


static const value_string vals_tag_code[] = {
	{ daap_mdcl, "dictionary (mdcl)" },
	{ daap_mstt, "status (mstt)" },
	{ daap_miid, "item id (miid)" },
	{ daap_minm, "item name (minm)" },
	{ daap_mikd, "item kind (mikd)" },
	{ daap_mper, "persistent id (mper)" },
	{ daap_mcon, "container (mcon)" },
	{ daap_mcti, "container item id (mcti)" },
	{ daap_mpco, "parent container id (mpco)" },
	{ daap_msts, "status string (msts)" },
	{ daap_mimc, "item count (mimc)" },
	{ daap_mrco, "returned count (mrco)" },
	{ daap_mtco, "specified total count (mtco)" },
	{ daap_mlcl, "listing (mlcl)" },
	{ daap_mlit, "listing item (mlit)" },
	{ daap_mbcl, "bag (mbcl)" },
	{ daap_msrv, "server info response (msrv)" },
	{ daap_msau, "authentication method (msau)" },
	{ daap_mslr, "login required (mslr)" },
	{ daap_mpro, "protocol version (mpro)" },
	{ daap_apro, "protocol (application?) version (apro)" },
	{ daap_msal, "supports auto-logout (msal)" },
	{ daap_msup, "supports update (msup)" },
	{ daap_mspi, "supports persistent ids (mspi)" },
	{ daap_msex, "supports extensions (msex)" },
	{ daap_msbr, "supports browse" },
	{ daap_msqy, "supports query" },
	{ daap_msix, "supports index" },
	{ daap_msrs, "supports resolve" },
	{ daap_mstm, "timeout interval" },
	{ daap_msdc, "databases count" },
	{ daap_mccr, "content codes response" },
	{ daap_mcnm, "content codes number" },
	{ daap_mcna, "content codes name" },
	{ daap_mcty, "content codes type" },
	{ daap_mlog, "login response" },
	{ daap_mlid, "session id" },
	{ daap_mupd, "update response" },
	{ daap_musr, "server revision" },
	{ daap_msur, "server revision" },
	{ daap_muty, "update type" },
	{ daap_mudl, "deleted id listing" },
	{ daap_avdb, "server databases" },
	{ daap_abro, "database browse" },
	{ daap_abal, "browse album listung" },
	{ daap_abar, "browse artist listing" },
	{ daap_abcp, "browse composer listing" },
	{ daap_abgn, "browse genre listing" },
	{ daap_adbs, "database songs" },
	{ daap_asal, "song album" },
	{ daap_asar, "song artist" },
	{ daap_asbt, "song beats-per-minute" },
	{ daap_asbr, "song bitrate" },
	{ daap_ascm, "song comment" },
	{ daap_asco, "song compilation" },
	{ daap_asda, "song date added" },
	{ daap_asdm, "song date modified" },
	{ daap_asdc, "song disccount" },
	{ daap_asdn, "song discnumber" },
	{ daap_asdb, "song disabled" },
	{ daap_aseq, "song eq preset" },
	{ daap_asfm, "song format" },
	{ daap_asgn, "song genre" },
	{ daap_asdt, "song description" },
	{ daap_asrv, "song relative volume" },
	{ daap_assr, "song sample rate" },
	{ daap_assz, "song size" },
	{ daap_asst, "song start time (miliseconds)" },
	{ daap_assp, "song stop time (miliseconds)" },
	{ daap_astm, "song time (miliseconds)" },
	{ daap_astc, "song track count" },
	{ daap_astn, "song track number" },
	{ daap_asur, "song user rating" },
	{ daap_asyr, "song year" },
	{ daap_asdk, "song data kind" },
	{ daap_asul, "song data url" },
	{ daap_aply, "database playlists" },
	{ daap_abpl, "base playlist" },
	{ daap_apso, "playlist songs" },
	{ daap_prsv, "resolve" },
	{ daap_arif, "resolveinfo" },
	{ daap_aeNV, "com.apple.itunes.norm-volume" },
	{ daap_aeSP, "com.apple.itunes.smart-playlist" },
	{ daap_arsv, "resolve" },
	{ daap_ascp, "song composer" },
	{ daap_ascd, "song codec type" },
	{ daap_ascs, "song codec subtype" },
	{ daap_msas, "authentication schemes" },
	{ daap_mctc, "container count" },
	{ daap_aeSV, "com.apple.itunes.music-sharing-version" },
	{ daap_aePI, "com.apple.itunes.itms-playlistid" },
	{ daap_aeCI, "com.apple.itunes.itms-composerid" },
	{ daap_aeGI, "com.apple.itunes.itms-genreid" },
	{ daap_aeAI, "com.apple.itunes.itms-artistid" },
	{ daap_aeSI, "com.apple.itunes.itms-songid" },
	{ daap_aeHV, "com.apple.itunes.has-video" },
	{ daap_aeHD, "com.apple.itunes.is-hd-video" },
	{ daap_aePC, "com.apple.itunes.is-podcast" },
	{ daap_aePP, "com.apple.itunes.is-podcast-playlist" },
	{ daap_asct, "song category" },
	{ daap_ascn, "song content description" },
	{ daap_aslc, "song long content description" },
	{ daap_asky, "song keywords" },
	{ daap_aeMK, "com.apple.itunes.mediakind" },
	{ daap_aeSG, "com.apple.itunes.saved-genius" },
	{ daap_apsm, "playlist shuffle mode" },
	{ daap_aprm, "playlist repeat mode" },
	{ daap_asgp, "song gapless" },
	{ daap_aePS, "com.apple.itunes.special-playlist" },
	{ daap_aeSN, "com.apple.itunes.series-name" },
	{ daap_aeNN, "com.apple.itunes.network-name" },
	{ daap_aeEN, "com.apple.itunes.episode-num-str" },
	{ daap_aeES, "com.apple.itunes.episode-sort" },
	{ daap_aeSU, "com.apple.itunes.season-num" },
	{ daap_ased, "song extra data" },
	{ daap_ashp, "song has been played" },
	{ daap_assn, "sort name" },
	{ daap_assa, "sort artist" },
	{ daap_assc, "sort composer" },
	{ daap_asss, "sort seriesname" },
	{ daap_asaa, "song album artist" },
	{ daap_aspu, "song podcast url" },
	{ daap_asbk, "song bookmarkable" },
	{ daap_asbo, "song bookmark" },
	{ daap_aeGH, "com.apple.itunes.gapless-heur" },
	{ daap_aeGD, "com.apple.itunes.gapless-enc-dr" },
	{ daap_aeGE, "com.apple.itunes.gapless-enc-del" },
	{ daap_aeGR, "com.apple.itunes.gapless-resy" },
	{ daap_aeGU, "com.apple.itunes.gapless-dur" },
	{ daap_aeCR, "com.apple.itunes.content-rating" },
	{ daap_asai, "song album id"},
	{ daap_asls, "song long size"},
	{ daap_meds, "edit commands supported" },
	{ 0, 	NULL}
};

/* Initialize the protocol and registered fields */
static int proto_daap = -1;
static int hf_daap_name = -1;
static int hf_daap_size = -1;

/* Initialize the subtree pointers */
static gint ett_daap = -1;
static gint ett_daap_sub = -1;

/* Forward declarations */
static int dissect_daap_one_tag(proto_tree *tree, tvbuff_t *tvb, guint offset, guint length);

static void
dissect_daap(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
   proto_item *ti;
   proto_tree *daap_tree;
   guint offset = 0;
   gboolean is_request = (pinfo->destport == TCP_PORT_DAAP);

   col_set_str(pinfo->cinfo, COL_PROTOCOL, "DAAP");

   /*
    * XXX - what if the body is gzipped?  This isn't the only protocol
    * running atop HTTP that might have a problem with that....
    */
   if (check_col(pinfo->cinfo, COL_INFO)) {
      if (is_request) {
	 col_set_str(pinfo->cinfo, COL_INFO, "DAAP Request");
      } else {
	 /* This is done in two functions on purpose. If the tvb_get_xxx()
	  * functions fail, at least something will be in the info column
	  */
	 col_set_str(pinfo->cinfo, COL_INFO, "DAAP Response");
	 col_append_fstr(pinfo->cinfo, COL_INFO, " [first tag: %s, size: %d]",
			 tvb_format_text(tvb, offset, 4),
			 tvb_get_ntohl(tvb, offset+4));
      }
   }

   if (tree) {
      ti = proto_tree_add_item(tree, proto_daap, tvb, 0, -1, FALSE);
      daap_tree = proto_item_add_subtree(ti, ett_daap);
      dissect_daap_one_tag(daap_tree, tvb, offset, 0);
   }
}

static int
dissect_daap_one_tag(proto_tree *tree, tvbuff_t *tvb, guint offset, guint length)
{
   guint32 tagname;
   guint32 tagsize;
   int new_offset;
   proto_item *ti = NULL;
   proto_item *ti2 = NULL;
   proto_tree *new_tree = NULL;

   do {
      if (!tvb_offset_exists(tvb, offset + 4))
	 break;

      tagname = tvb_get_ntohl(tvb, offset);
      tagsize = tvb_get_ntohl(tvb, offset+4);
      tvb_ensure_bytes_exist(tvb, offset, tagsize+8);
      ti = proto_tree_add_text(tree, tvb, offset, tagsize+8,
			       "Tag: %s, %d bytes",
			       val_to_str(tagname,vals_tag_code, "Unknown tag (%d)"),
			       tagsize);

      ti2 = proto_tree_add_item(tree, hf_daap_name, tvb, offset, 4, FALSE);
      PROTO_ITEM_SET_HIDDEN(ti2);
      ti2 = proto_tree_add_item(tree, hf_daap_size, tvb, offset+4, 4, FALSE);
      PROTO_ITEM_SET_HIDDEN(ti2);
      offset += 8;
      length -= 8;

      switch (tagname) {
      case daap_mcon:
      case daap_msrv:
      case daap_mccr:
      case daap_mdcl:
      case daap_mlog:
      case daap_mupd:
      case daap_avdb:
      case daap_mlcl:
      case daap_mlit:
      case daap_mbcl:
      case daap_adbs:
      case daap_aply:
      case daap_apso:
      case daap_mudl:
      case daap_abro:
      case daap_abar:
      case daap_arsv:
      case daap_abal:
      case daap_abcp:
      case daap_abgn:
      case daap_prsv:
      case daap_arif:
	 /* Container tags */
	 new_tree = proto_item_add_subtree(ti, ett_daap_sub);
	 new_offset = dissect_daap_one_tag(new_tree, tvb, offset,
					   tagsize);
	 break;
      case daap_minm:
      case daap_msts:
      case daap_mcnm:
      case daap_mcna:
      case daap_asal:
      case daap_asar:
      case daap_ascm:
      case daap_asfm:
      case daap_aseq:
      case daap_asgn:
      case daap_asdt:
      case daap_asul:
      case daap_ascp:
      case daap_asct:
      case daap_ascn:
      case daap_aslc:
      case daap_asky:
      case daap_aeSN:
      case daap_aeNN:
      case daap_aeEN:
      case daap_assn:
      case daap_assa:
      case daap_assl:
      case daap_assc:
      case daap_asss:
      case daap_asaa:
      case daap_aspu:
      case daap_aeCR:
	 /* Tags contain strings */
	 proto_item_append_text(ti, ", Data: %s",
				tvb_format_text(tvb, offset, tagsize));
	 break;
      case daap_mper:
      case daap_aeGR:
      case daap_aeGU:
      case daap_asai:
      case daap_asls:
	 /* Tags conain uint64 */
	 proto_item_append_text(ti, ", Persistent Id: %" G_GINT64_MODIFIER "u",
				tvb_get_ntoh64(tvb, offset));
	 break;
      case daap_mstt:
	 proto_item_append_text(ti, ", Status: %d",
				tvb_get_ntohl(tvb, offset));
	 break;
      case daap_musr:
      case daap_msur:
	 proto_item_append_text(ti, ", Revision: %d",
				tvb_get_ntohl(tvb, offset));
	 break;
      case daap_miid:
      case daap_mcti:
      case daap_mpco:
      case daap_mlid:
	 proto_item_append_text(ti, ", Id: %d",
				tvb_get_ntohl(tvb, offset));
	 break;
      case daap_mrco:
      case daap_mtco:
      case daap_mimc:
      case daap_msdc:
      case daap_mctc:
	 proto_item_append_text(ti, ", Count: %d",
				tvb_get_ntohl(tvb, offset));
	 break;
      case daap_mstm:
	 proto_item_append_text(ti, ", Timeout: %d seconds",
				tvb_get_ntohl(tvb, offset));
	 break;
      case daap_asda:
      case daap_asdm:
      case daap_assr:
      case daap_assz:
      case daap_asst:
      case daap_assp:
      case daap_astm:
      case daap_aeNV:
      case daap_ascd:
      case daap_ascs:
      case daap_aeSV:
      case daap_aePI:
      case daap_aeCI:
      case daap_aeGI:
      case daap_aeAI:
      case daap_aeSI:
      case daap_aeES:
      case daap_asbo:
      case daap_aeGH:
      case daap_aeGD:
      case daap_aeGE:
      case daap_meds:
	 /* Tags conain uint32 */
	 proto_item_append_text(ti, ", Data: %d",
				tvb_get_ntohl(tvb, offset));
	 break;

      case daap_mcty:
      case daap_asbt:
      case daap_asbr:
      case daap_asdc:
      case daap_asdn:
      case daap_astc:
      case daap_astn:
      case daap_asyr:
      case daap_ased:
	 /* Tags conain uint16 */
	 proto_item_append_text(ti, ", Data: %d",
				tvb_get_ntohs(tvb, offset));
	 break;

      case daap_mikd:
      case daap_msau:
      case daap_msty:
      case daap_asrv:
      case daap_asur:
      case daap_asdk:
      case daap_muty:
      case daap_msas:
      case daap_aeHV:
      case daap_aeHD:
      case daap_aePC:
      case daap_aePP:
      case daap_aeMK:
      case daap_aeSG:
      case daap_apsm:
      case daap_aprm:
      case daap_asgp:
      case daap_aePS:
	 /* Tags conain uint8 */
	 proto_item_append_text(ti, ", Data: %d",
				tvb_get_guint8(tvb, offset));

	 break;

      case daap_mslr:
      case daap_msal:
      case daap_msup:
      case daap_mspi:
      case daap_msex:
      case daap_msbr:
      case daap_msqy:
      case daap_msix:
      case daap_msrs:
      case daap_asco:
      case daap_asdb:
      case daap_abpl:
      case daap_aeSP:
      case daap_asbk:
	 /* Tags ARE boolean. Data is (uint8), but it seems
	  * the value is always zero. So, if the tag is present
	  * the "bool" is true.
	  */
	 proto_item_append_text(ti, ", Data: True");
	 break;

      case daap_mpro:
      case daap_apro:
	 /* Tags conain version (uint32) */
	 proto_item_append_text(ti, ", Version: %d.%d.%d.%d",
				tvb_get_guint8(tvb, offset),
				tvb_get_guint8(tvb, offset+1),
				tvb_get_guint8(tvb, offset+2),
				tvb_get_guint8(tvb, offset+3));
	 break;

      default:
	 break;
      }
      offset += tagsize;
      length -= tagsize;
   } while (length > 0);
   return offset;
}


/* Register the protocol with Wireshark */
void
proto_register_daap(void)
{

   static hf_register_info hf[] = {
      { &hf_daap_name,
	{ "Name", "daap.name", FT_STRING, BASE_NONE, NULL, 0x0,
	  "Tag Name", HFILL}
      },
      { &hf_daap_size,
	{ "Size", "daap.size", FT_UINT32, BASE_DEC, NULL, 0x0,
	  "Tag Size", HFILL }
      }
   };

   static gint *ett[] = {
      &ett_daap,
      &ett_daap_sub,
   };

   proto_daap = proto_register_protocol("Digital Audio Access Protocol",
					"DAAP", "daap");

   proto_register_field_array(proto_daap, hf, array_length(hf));
   proto_register_subtree_array(ett, array_length(ett));
}


/* If this dissector uses sub-dissector registration add a registration routine.
   This format is required because a script is used to find these routines and
   create the code that calls these routines.
*/
void
proto_reg_handoff_daap(void)
{
    dissector_handle_t daap_handle;

    daap_handle = create_dissector_handle(dissect_daap, proto_daap);
    http_dissector_add(TCP_PORT_DAAP, daap_handle);
}
