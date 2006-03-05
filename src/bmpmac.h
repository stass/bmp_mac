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
 * Author: Stanislav Sedov <ssedov@mbsd.msk.ru>
 *
 * $Id: bmpmac.h,v 1.1 2006/03/05 18:50:53 stas Exp $
 */

#ifndef _MAC_H_
#define _MAC_H_

typedef struct {
	int		playing;	/* Play status flag */
	char		*title;		/* BMP song title */

	AFormat		s_format; 	/* DATA format */
	uint		bpb;		/* bytes per block */
	uint		nchan;		/* number of channels */
	uint		s_rate;		/* sample rate */
	uint		len;		/* song length */
	uint		blk_align;	/* alignment */
    
	int		seek_to;	/* position we must seek to */
	uint		p_time;		/* pause time */

	IAPEDecompress	*dec_sc;

	pthread_t	tid;
} bmpmac_softc_t;

#ifdef __cplusplus
extern "C"{
#endif

static void	bmpmac_init	__P(());
static int	bmpmac_probe	__P((char *filename));
static void	bmpmac_play	__P((char *filename));
static void	bmpmac_stop	__P(());
static void	bmpmac_pause	__P((short p));
static void	bmpmac_seek	__P((int time));
static int	bmpmac_get_time	__P(());
static void	bmpmac_gsinfo	__P((char *filename, char **title, int *len));

#ifdef __cplusplus
}
#endif

#endif
