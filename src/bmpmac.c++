/*-
 * Copyright (c) 2006 MBSD labs, http://mbsd.msk.ru/
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Parts of this file are heavily based on code from xmms-mac plugin and
 * MAC codec so under the GPL and Monkey's Audio Source Code License Agreement.
 *
 * Author: Stanislav Sedov <ssedov@mbsd.msk.ru>
 *
 * $Id: bmpmac.c++,v 1.1 2006/03/05 18:50:53 stas Exp $
 */

#include "config.h"

/* system headers */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <pthread.h>
#include <assert.h>
#include <libgen.h>

/* BMP headers */
#include <bmp/plugin.h>
#include <bmp/configfile.h>
#include <bmp/util.h>
#include <bmp/titlestring.h>

/* MAC headers */
#include <mac/All.h>
#include <mac/MACLib.h>
#include <mac/APETag.h>
#include <mac/APEInfo.h>
#include <mac/CharacterHelper.h>

#include "bmpmac.h"

#undef __BMP_DEBUG__
#ifdef __BMP_DEBUG__
#	define ASSERT(x) assert(x)
#endif

#define BUFSIZE	512

/* General plugin structure */
InputPlugin bmpmac_ip = {
	NULL,			/* reserved		*/
	NULL,			/* reserved		*/
	NULL,			/* description		*/
	NULL,		 	/* init 		*/
	NULL,			/* about box		*/
	NULL,			/* configure box	*/
	bmpmac_probe,		/* probe file		*/
	NULL,			/* scan dir		*/
	bmpmac_play,		/* play			*/
	bmpmac_stop,		/* stop			*/
	bmpmac_pause,		/* pause		*/
	bmpmac_seek,		/* seek			*/
	NULL,			/* set the equalizer	*/
	bmpmac_get_time,	/* get the time		*/
	NULL,			/* get volume		*/
	NULL,			/* set volume		*/
	NULL,			/* OBSOLETE!		*/
	NULL,			/* OBSOLETE!		*/
	NULL,			/* get visual data	*/
	NULL,			/* get song info	*/
	NULL,			/* get add song info	*/
	bmpmac_gsinfo,		/* get the title string */
	NULL,			/* file info box	*/
	NULL			/* output plugin	*/
};

static bmpmac_softc_t	bmpmac_sc;

#ifdef __cplusplus
extern "C"{
#endif

/* Must return non-NULL pointer */
static char *
get_tag_info(
	CAPETag	*tag,
	wchar_t	*field)
{
	CAPETagField	*pTagField;
	const char	*fieldValue;
	char		*value;

	ASSERT(tag);
	ASSERT(field);

	pTagField = tag->GetTagField(field);
	if (pTagField == NULL)
		return strdup("");

	fieldValue = pTagField->GetFieldValue();
	if (tag->GetAPETagVersion() == CURRENT_APE_TAG_VERSION)
		value = GetANSIFromUTF8((unsigned char *)fieldValue);
#warning Need to duplicate?
	else
		value = strdup(fieldValue);

	return value;
}

static char *
mac_format_title_string(
	char	*filename,
	CAPETag	*tag)
{
	char		*ret;
	TitleInput	*input;
	int		hasTag;

	if (tag == NULL ||
	    (tag->GetHasID3Tag() == 0 && tag->GetHasAPETag() == 0))
		return strdup(filename);

	/* Some strange bmp operation */
        XMMS_NEW_TITLEINPUT(input);

	input->performer = get_tag_info(tag, APE_TAG_FIELD_ARTIST);
	input->album_name = get_tag_info(tag, APE_TAG_FIELD_ALBUM);
	input->track_name = get_tag_info(tag, APE_TAG_FIELD_TITLE);
	input->track_number = atoi(get_tag_info(tag, APE_TAG_FIELD_TRACK));
	input->year = strtol(get_tag_info(tag, APE_TAG_FIELD_YEAR),
				(char **)NULL, 10);
	input->genre = get_tag_info(tag, APE_TAG_FIELD_GENRE);
	input->comment = get_tag_info(tag, APE_TAG_FIELD_COMMENT);
	input->file_name = basename(filename);
	if (input->file_name == NULL);
		input->file_name = filename;
	input->file_path = filename;
	input->file_ext = strrchr(filename, '.');
		if (input->file_ext != NULL)
			input->file_ext++;
                                                                                
	ret = xmms_get_titlestring(xmms_get_gentitle_format(), input);
	free(input);
#warning Need to free components
	
	if (ret != NULL) {
		return ret;
	}
	else
		return strdup(filename);
}

static int
bmpmac_sc_init(char *filename)
{
	int		ret;
	IAPEDecompress	*dec_sc = NULL;
	wchar_t		*pUTF16 = NULL;
	CAPETag		*tag = NULL;

	bzero(&bmpmac_sc, sizeof(bmpmac_softc_t));
	bmpmac_sc.seek_to = -1;

	pUTF16 = GetUTF16FromANSI(filename);
	if (pUTF16 == NULL)
		goto fail;

	dec_sc = CreateIAPEDecompress(pUTF16, &ret);
	free(pUTF16);

	if (ret != ERROR_SUCCESS)
		goto fail;

	ASSERT(dec_sc);

	bmpmac_sc.dec_sc = dec_sc;

	/* this dirty thing is from official example */
	tag = (CAPETag *)dec_sc->GetInfo(APE_INFO_TAG);
	if (tag == NULL)
		goto fail;

	bmpmac_sc.title = mac_format_title_string(filename, tag);
	bmpmac_sc.s_rate = dec_sc->GetInfo(APE_INFO_SAMPLE_RATE);
	bmpmac_sc.bpb = dec_sc->GetInfo(APE_INFO_BITS_PER_SAMPLE);
	bmpmac_sc.nchan = dec_sc->GetInfo(APE_INFO_CHANNELS);
	bmpmac_sc.len = dec_sc->GetInfo(APE_DECOMPRESS_LENGTH_MS);
	bmpmac_sc.blk_align = dec_sc->GetInfo(APE_INFO_BLOCK_ALIGN);
	bmpmac_sc.s_format = (bmpmac_sc.bpb == 16) ? FMT_S16_LE : FMT_S8;
	bmpmac_sc.seek_to = -1;
	bmpmac_sc.tid = 0;

	return 0;

fail:
	if (dec_sc != NULL)
		free(dec_sc);

	return 1;
}                                                                               

static void *
work_loop(void *arg)
{                                                                               
	uint8_t		*data;
	int		size, bytes;
	int		ret;

	data = (uint8_t *)malloc(BUFSIZE * bmpmac_sc.blk_align);
	if (data == NULL)
		return NULL;

	while (bmpmac_sc.playing) {
		if (bmpmac_sc.seek_to != -1) {
			bmpmac_sc.dec_sc->Seek(bmpmac_sc.seek_to *	\
						bmpmac_sc.s_rate);
			bmpmac_ip.output->flush(bmpmac_sc.seek_to * 1000);
			bmpmac_sc.seek_to = -1;
		}

		ret = bmpmac_sc.dec_sc->GetData((char *)data, BUFSIZE, &size);
		if (size <= 0)
			break;

		bytes = size * bmpmac_sc.blk_align;
		bmpmac_ip.add_vis_pcm(bmpmac_ip.output->written_time(),
					bmpmac_sc.s_format,
					bmpmac_sc.nchan,
					bytes, data);

		while (bmpmac_ip.output->buffer_free() < bytes
			&& bmpmac_sc.playing
			&& bmpmac_sc.seek_to == -1)
			xmms_usleep(10000);

		if (bmpmac_sc.playing && bmpmac_sc.seek_to == -1)
			bmpmac_ip.output->write_audio(data, bytes);
	}
	
	if (bmpmac_sc.playing && bmpmac_ip.output->buffer_playing())
		xmms_usleep(30000);

	bmpmac_sc.playing = 0;

	
	free(data);
	pthread_exit(NULL);

	/* Not achieved */
}

/****************************************************************************
 *			Plugin-related functions			    *
 ****************************************************************************/

InputPlugin *get_iplugin_info()
{

    bmpmac_ip.description = strdup("Monkey's audio plugin");

    return &bmpmac_ip;
}

static int bmpmac_probe(char *filename)
{
	char *	ext;

	if ((ext = strrchr(filename, '.')) != NULL &&
	    (strcasecmp(ext, ".mac") == 0 ||
	    strcasecmp(ext, ".ape") == 0 ||
	    strcasecmp(ext, ".apl") == 0))
		return 1;

	return 0;
}

static void
bmpmac_play(char *filename)
{
	uint32_t	bitrate;
	int		ret;

	ret = bmpmac_sc_init(filename); /* fills in softc */
	if (ret != 0)
		return; /* we can't play */

	ret = bmpmac_ip.output->open_audio(bmpmac_sc.s_format,
						bmpmac_sc.s_rate,
						bmpmac_sc.nchan);
	if(ret == 0)
		return;

	ASSERT(bmpmac_sc.dec_sc);

	bitrate = bmpmac_sc.dec_sc->GetInfo(APE_DECOMPRESS_AVERAGE_BITRATE);
	bmpmac_ip.set_info(bmpmac_sc.title,
				bmpmac_sc.len,
				bitrate * 1000,
				bmpmac_sc.s_rate,
				bmpmac_sc.nchan);

	ret = pthread_create(&bmpmac_sc.tid, NULL, work_loop, NULL);
	if (ret == 0)
		bmpmac_sc.playing = 1;
}

static void
bmpmac_stop()
{

	bmpmac_sc.playing = 0;

	if (bmpmac_sc.p_time != 0)
		bmpmac_pause(0);

	pthread_join(bmpmac_sc.tid, NULL);
	bmpmac_ip.output->close_audio();

	if (bmpmac_sc.dec_sc != NULL) {
		delete bmpmac_sc.dec_sc;
		bmpmac_sc.dec_sc = NULL;
	}

}

static void
bmpmac_pause(short p)
{

	bmpmac_sc.p_time = p;
	bmpmac_ip.output->pause(p);
}

static void
bmpmac_seek(int time)
{

	if (bmpmac_sc.p_time != 0)
		bmpmac_pause(0);

	bmpmac_sc.seek_to = time;

	while (bmpmac_sc.seek_to != -1)
		xmms_usleep(10000);

	if (bmpmac_sc.p_time != 0)
		bmpmac_pause(0);
}

static int bmpmac_get_time(void)
{

	bmpmac_ip.output->buffer_free();
	if (bmpmac_sc.playing == 0)
		return -1;

	return bmpmac_ip.output->output_time();
}

static void
bmpmac_gsinfo(
	char	*filename,
	char	**title,
	int	*length)
{
	wchar_t		*pUTF16Name;
	IAPEDecompress	*pDec;
        CAPETag		*apeTag;
	int		ret;

	if (filename == NULL)
		return;

	pUTF16Name = GetUTF16FromANSI(filename);
	if (pUTF16Name == NULL)
		return;

	pDec = CreateIAPEDecompress(pUTF16Name, &ret);

	if (ret != ERROR_SUCCESS) {
		if (title != NULL)
			*title = strdup("Invalid MAC header");
		if (length != NULL)
			*length = -1;

		return;
	}

	ASSERT(pDec != NULL);

	if (title != NULL) {
		apeTag = (CAPETag *)pDec->GetInfo(APE_INFO_TAG);
		*title = mac_format_title_string(filename, apeTag);
	}

	if (length != 0) {
		*length = pDec->GetInfo(APE_DECOMPRESS_LENGTH_MS);
	}

	delete pDec;
}

#ifdef __cplusplus
}
#endif
