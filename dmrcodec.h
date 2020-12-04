/*
	Copyright (C) 2019 Doug McLain

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef DMRCODEC_H
#define DMRCODEC_H

#include <QObject>
#include <QtNetwork>
#include "audioengine.h"
#include "serialambe.h"
#include "mbedec.h"
#include "mbeenc.h"
#ifdef USE_FLITE
#include <flite/flite.h>
#endif
#include <inttypes.h>
#include "DMRData.h"
#include "cbptc19696.h"

class DMRCodec : public QObject
{
	Q_OBJECT
public:
    DMRCodec(QString callsign, uint32_t dmrid, QString password, QString options, uint32_t m_repeaterId, uint32_t dstid, QString host, uint32_t port, QString vocoder, QString audioin, QString audioout);
	~DMRCodec();
	unsigned char * get_eot();
	uint8_t get_status(){ return m_status; }
	void set_srcid(uint32_t s){m_srcid = s;}
	void set_dstid(uint32_t d){m_dstid = d;}
	void set_cc(uint32_t cc){m_colorcode = cc;}
	void set_slot(uint32_t s){m_slot = s;}
	void set_calltype(uint8_t c){m_flco = FLCO(c);}
	uint32_t get_src() { return m_srcid; }
	uint32_t get_dst() { return m_dstid; }
	uint32_t get_gw() { return m_gwid; }
	QString get_host() { return m_host; }
	int get_port() { return m_port; }
	int get_fn() { return m_fn; }
	int get_cnt() { return m_cnt; }
	bool get_hwrx() { return m_hwrx; }
	bool get_hwtx() { return m_hwtx; }
signals:
	void update();
private slots:
	void start_tx();
	void stop_tx();
	void deleteLater();
	void process_udp();
    void send_options();
	void process_rx_data();
	void get_ambe();
	void send_ping();
	void send_connect();
	void send_disconnect();

	void transmit();
	void hostname_lookup(QHostInfo i);
	void dmr_tgid_changed(unsigned int id) { m_txdstid = id; }
	void dmrpc_state_changed(int pc){ m_pc = (pc ? true : false); }
	void input_src_changed(int id, QString t) { m_ttsid = id; m_ttstext = t; }
	void swrx_state_changed(int s) {m_hwrx = !s; }
	void swtx_state_changed(int s) {m_hwtx = !s; }
	void send_frame();
	void in_audio_vol_changed(qreal);
	void out_audio_vol_changed(qreal);
private:
	enum{
		DISCONNECTED,
		CONNECTING,
		DMR_AUTH,
		DMR_CONF,
		DMR_OPTS,
		CONNECTED_RW,
		CONNECTED_RO
	} m_status;
	QUdpSocket *m_udp = nullptr;
	QHostAddress m_address;
	QString m_callsign;
	uint32_t m_dmrid;
	QString m_password;
    QString m_options;
    uint32_t m_repeaterId;
	uint32_t m_srcid;
	uint32_t m_dstid;
	uint32_t m_txdstid;
	uint32_t m_gwid;
	QString m_hostname;
	QString m_host;
	int m_port;
	uint16_t m_fn;
	bool m_tx;
	uint16_t m_txcnt;
	uint32_t m_rxcnt;
	uint8_t m_ttsid;
	QString m_ttstext;
	int m_cnt;
	uint32_t m_transmitcnt;
	QString m_vocoder;
	SerialAMBE *m_ambedev;
	QTimer *m_rxtimer;
	bool m_hwrx;
	bool m_hwtx;
	uint8_t packet_size;
	uint16_t m_ttscnt;
	uint8_t m_ambe[27];
	QQueue<char> m_rxambeq;
	QQueue<char> m_ambeq;

#ifdef USE_FLITE
	cst_voice *voice_slt;
	cst_voice *voice_kal;
	cst_voice *voice_awb;
	cst_voice *voice_rms;
	cst_wave *tts_audio;
#endif
	uint32_t m_defsrcid;
	QString m_type;

	uint16_t m_streamid;
	QTimer *m_ping_timer;
	QTimer *m_txtimer;
	AudioEngine *m_audio;
	MBEDecoder *m_mbedec;
	MBEEncoder *m_mbeenc;

	uint8_t m_dmrFrame[55];
	uint8_t m_dataType;
	uint32_t m_colorcode;
	uint32_t m_slot;
	uint32_t m_dmrcnt;
	bool m_pc;
	FLCO m_flco;
	CBPTC19696 m_bptc;
	bool m_raw[128U];
	bool m_data[72U];
	QString m_audioin;
	QString m_audioout;

	void byteToBitsBE(uint8_t byte, bool* bits);
	void bitsToByteBE(const bool* bits, uint8_t& byte);

	void build_frame();
	void encode_header();
	void encode_data();
	void encode16114(bool* d);
	void encode_qr1676(uint8_t* data);
	void get_slot_data(uint8_t* data);
	void lc_get_data(uint8_t*);
	void lc_get_data(bool* bits);
	void encode_embedded_data();
	uint8_t get_embedded_data(uint8_t* data, uint8_t n);
	void get_emb_data(uint8_t* data, uint8_t lcss);
	void full_lc_encode(uint8_t* data, uint8_t type);
	void addDMRDataSync(uint8_t* data, bool duplex);
	void addDMRAudioSync(uint8_t* data, bool duplex);
};

#endif // DMRCODEC_H
