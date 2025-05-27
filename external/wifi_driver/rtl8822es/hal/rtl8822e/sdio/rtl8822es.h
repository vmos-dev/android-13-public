/******************************************************************************
 *
 * Copyright(c) 2015 - 2017 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/
#ifndef _RTL8822ES_H_
#define _RTL8822ES_H_

#include <drv_types.h>		/* PADAPTER, struct dvobj_priv and etc. */


/* rtl8822es_halinit.c */
u32 rtl8822es_init(PADAPTER);
u32 rtl8822es_deinit(PADAPTER adapter);
void rtl8822es_init_default_value(PADAPTER);

/* rtl8822es_halmac.c */
int rtl8822es_halmac_init_adapter(PADAPTER);

/* rtl8822es_io.c */
u32 rtl8822es_read_port(struct dvobj_priv *, u32 cnt, u8 *mem);
u32 rtl8822es_write_port(struct dvobj_priv *, u32 cnt, u8 *mem);

/* rtl8822es_led.c */
void rtl8822es_initswleds(PADAPTER);
void rtl8822es_deinitswleds(PADAPTER);

/* rtl8822es_xmit.c */
s32 rtl8822es_init_xmit_priv(PADAPTER);
void rtl8822es_free_xmit_priv(PADAPTER);
#ifdef CONFIG_RTW_MGMT_QUEUE 
s32 rtl8822es_hal_mgmt_xmit_enqueue(PADAPTER, struct xmit_frame *);
#endif
s32 rtl8822es_hal_xmit_enqueue(PADAPTER, struct xmit_frame *);
s32 rtl8822es_hal_xmit(PADAPTER, struct xmit_frame *);
s32 rtl8822es_mgnt_xmit(PADAPTER, struct xmit_frame *);
s32 rtl8822es_xmit_buf_handler(PADAPTER);
thread_return rtl8822es_xmit_thread(thread_context);

/* rtl8822es_recv.c */
s32 rtl8822es_init_recv_priv(PADAPTER);
void rtl8822es_free_recv_priv(PADAPTER);
_pkt *rtl8822es_alloc_recvbuf_skb(struct recv_buf *, u32 size);
void rtl8822es_free_recvbuf_skb(struct recv_buf *);
s32 rtl8822es_recv_hdl(_adapter *adapter);
void rtl8822es_rxhandler(PADAPTER, struct recv_buf *);

/* rtl8822es_ops.c */
void rtl8822es_get_interrupt(PADAPTER, u32 *hisr, u32 *rx_len);
void rtl8822es_clear_interrupt(PADAPTER, u32 hisr);

#endif /* _RTL8822ES_H_ */
