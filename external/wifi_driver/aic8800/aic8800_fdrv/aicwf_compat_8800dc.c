#include "rwnx_main.h"
#include "rwnx_msg_tx.h"
#include "reg_access.h"

#define RWNX_MAC_RF_PATCH_BASE_NAME_8800DC     "fmacfw_rf_patch_8800dc"
#define RWNX_MAC_RF_PATCH_NAME_8800DC RWNX_MAC_RF_PATCH_BASE_NAME_8800DC".bin"
#define FW_USERCONFIG_NAME_8800DC         "aic_userconfig_8800dc.txt"
#define FW_USERCONFIG_NAME_8800DW         "aic_userconfig_8800dw.txt"


int rwnx_plat_bin_fw_upload_2(struct rwnx_hw *rwnx_hw, u32 fw_addr,
                               char *filename);
int rwnx_request_firmware_common(struct rwnx_hw *rwnx_hw,
	u32** buffer, const char *filename);
void rwnx_plat_userconfig_parsing2(char *buffer, int size);

void rwnx_release_firmware_common(u32** buffer);

u32 wifi_txgain_table_24g_8800dcdw[32] =
{
    0xA4B22189, //index 0
    0x00007825,
    0xA4B2214B, //index 1
    0x00007825,
    0xA4B2214F, //index 2
    0x00007825,
    0xA4B221D5, //index 3
    0x00007825,
    0xA4B221DC, //index 4
    0x00007825,
    0xA4B221E5, //index 5
    0x00007825,
    0xAC9221E5, //index 6
    0x00006825,
    0xAC9221EF, //index 7
    0x00006825,
    0xBC9221EE, //index 8
    0x00006825,
    0xBC9221FF, //index 9
    0x00006825,
    0xBC9221FF, //index 10
    0x00004025,
    0xB792203F, //index 11
    0x00004026,
    0xDC92203F, //index 12
    0x00004025,
    0xE692203F, //index 13
    0x00004025,
    0xFF92203F, //index 14
    0x00004035,
    0xFFFE203F, //index 15
    0x00004832
};

u32 wifi_txgain_table_24g_1_8800dcdw[32] =
{
    0x090E2011, //index 0
    0x00004001,
    0x090E2015, //index 1
    0x00004001,
    0x090E201B, //index 2
    0x00004001,
    0x110E2018, //index 3
    0x00004001,
    0x110E201E, //index 4
    0x00004001,
    0x110E2023, //index 5
    0x00004001,
    0x190E2021, //index 6
    0x00004001,
    0x190E202B, //index 7
    0x00004001,
    0x210E202B, //index 8
    0x00004001,
    0x230E2027, //index 9
    0x00004001,
    0x230E2031, //index 10
    0x00004001,
    0x240E2039, //index 11
    0x00004001,
    0x260E2039, //index 12
    0x00004001,
    0x2E0E203F, //index 13
    0x00004001,
    0x368E203F, //index 14
    0x00004001,
    0x3EF2203F, //index 15
    0x00004001
};

u32 wifi_rxgain_table_24g_20m_8800dcdw[64] = {
    0x82f282d1,//index 0
    0x9591a324,
    0x80808419,
    0x000000f0,
    0x42f282d1,//index 1
    0x95923524,
    0x80808419,
    0x000000f0,
    0x22f282d1,//index 2
    0x9592c724,
    0x80808419,
    0x000000f0,
    0x02f282d1,//index 3
    0x9591a324,
    0x80808419,
    0x000000f0,
    0x06f282d1,//index 4
    0x9591a324,
    0x80808419,
    0x000000f0,
    0x0ef29ad1,//index 5
    0x9591a324,
    0x80808419,
    0x000000f0,
    0x0ef29ad3,//index 6
    0x95923524,
    0x80808419,
    0x000000f0,
    0x0ef29ad7,//index 7
    0x9595a324,
    0x80808419,
    0x000000f0,
    0x02f282d2,//index 8
    0x95951124,
    0x80808419,
    0x000000f0,
    0x02f282f4,//index 9
    0x95951124,
    0x80808419,
    0x000000f0,
    0x02f282e6,//index 10
    0x9595a324,
    0x80808419,
    0x000000f0,
    0x02f282e6,//index 11
    0x9599a324,
    0x80808419,
    0x000000f0,
    0x02f282e6,//index 12
    0x959da324,
    0x80808419,
    0x000000f0,
    0x02f282e6,//index 13
    0x959f5924,
    0x80808419,
    0x000000f0,
    0x06f282e6,//index 14
    0x959f5924,
    0x80808419,
    0x000000f0,
    0x0ef29ae6,//index 15
    0x959f592c,//////0x959f5924,           //loft [35:34]=3
    0x80808419,
    0x000000f0
};

u32 wifi_rxgain_table_24g_40m_8800dcdw[64] = {
    0x83428151,//index 0
    0x9631a328,
    0x80808419,
    0x000000f0,
    0x43428151,//index 1
    0x96323528,
    0x80808419,
    0x000000f0,
    0x23428151,//index 2
    0x9632c728,
    0x80808419,
    0x000000f0,
    0x03428151,//index 3
    0x9631a328,
    0x80808419,
    0x000000f0,
    0x07429951,//index 4
    0x9631a328,
    0x80808419,
    0x000000f0,
    0x0f42d151,//index 5
    0x9631a328,
    0x80808419,
    0x000000f0,
    0x0f42d153,//index 6
    0x96323528,
    0x80808419,
    0x000000f0,
    0x0f42d157,//index 7
    0x9635a328,
    0x80808419,
    0x000000f0,
    0x03428152,//index 8
    0x96351128,
    0x80808419,
    0x000000f0,
    0x03428174,//index 9
    0x96351128,
    0x80808419,
    0x000000f0,
    0x03428166,//index 10
    0x9635a328,
    0x80808419,
    0x000000f0,
    0x03428166,//index 11
    0x9639a328,
    0x80808419,
    0x000000f0,
    0x03428166,//index 12
    0x963da328,
    0x80808419,
    0x000000f0,
    0x03428166,//index 13
    0x963f5928,
    0x80808419,
    0x000000f0,
    0x07429966,//index 14
    0x963f5928,
    0x80808419,
    0x000000f0,
    0x0f42d166,//index 15
    0x963f5928,
    0x80808419,
    0x000000f0
};

int aicwf_set_rf_config_8800dc(struct rwnx_hw *rwnx_hw, struct mm_set_rf_calib_cfm *cfm){
	int ret = 0;

	if ((ret = rwnx_send_txpwr_lvl_req(rwnx_hw))) {
		return -1;
	}

	if ((ret = rwnx_send_txpwr_ofst_req(rwnx_hw))) {
		return -1;
	}


	if (testmode == 0) {
		if ((ret = rwnx_send_rf_config_req(rwnx_hw, 0,	1, (u8_l *)wifi_txgain_table_24g_8800dcdw, 128)))
			return -1;

        if ((ret = rwnx_send_rf_config_req(rwnx_hw, 16,	1, (u8_l *)wifi_txgain_table_24g_1_8800dcdw, 128)))
			return -1;

		if ((ret = rwnx_send_rf_config_req(rwnx_hw, 0,	0, (u8_l *)wifi_rxgain_table_24g_20m_8800dcdw, 256)))
			return -1;

		if ((ret = rwnx_send_rf_config_req(rwnx_hw, 32,  0, (u8_l *)wifi_rxgain_table_24g_40m_8800dcdw, 256)))
			return -1;

		if ((ret = rwnx_send_rf_calib_req(rwnx_hw, cfm))) {
			return -1;
		}
	}

	return 0 ;
}

int	rwnx_plat_userconfig_load_8800dc(struct rwnx_hw *rwnx_hw){
    int size;
    u32 *dst=NULL;
    char *filename = FW_USERCONFIG_NAME_8800DC;

    AICWFDBG(LOGINFO, "userconfig file path:%s \r\n", filename);

    /* load file */
    size = rwnx_request_firmware_common(rwnx_hw, &dst, filename);
    if (size <= 0) {
            AICWFDBG(LOGERROR, "wrong size of firmware file\n");
            dst = NULL;
            return 0;
    }

	/* Copy the file on the Embedded side */
    AICWFDBG(LOGINFO, "### Load file done: %s, size=%d\n", filename, size);

	rwnx_plat_userconfig_parsing2((char *)dst, size);

    rwnx_release_firmware_common(&dst);

    AICWFDBG(LOGINFO, "userconfig download complete\n\n");
    return 0;

}

int	rwnx_plat_userconfig_load_8800dw(struct rwnx_hw *rwnx_hw){
    int size;
    u32 *dst=NULL;
    char *filename = FW_USERCONFIG_NAME_8800DC;

    AICWFDBG(LOGINFO, "userconfig file path:%s \r\n", filename);

    /* load file */
    size = rwnx_request_firmware_common(rwnx_hw, &dst, filename);
    if (size <= 0) {
            AICWFDBG(LOGERROR, "wrong size of firmware file\n");
            dst = NULL;
            return 0;
    }

	/* Copy the file on the Embedded side */
    AICWFDBG(LOGINFO, "### Load file done: %s, size=%d\n", filename, size);

	rwnx_plat_userconfig_parsing2((char *)dst, size);

    rwnx_release_firmware_common(&dst);

    AICWFDBG(LOGINFO, "userconfig download complete\n\n");
    return 0;

}

