/*
* Customer code to add GPIO control during WLAN start/stop
* Copyright (C) 1999-2014, Broadcom Corporation
* 
*      Unless you and Broadcom execute a separate written software license
* agreement governing use of this software, this software is licensed to you
* under the terms of the GNU General Public License version 2 (the "GPL"),
* available at http://www.broadcom.com/licenses/GPLv2.php, with the
* following added to such license:
* 
*      As a special exception, the copyright holders of this software give you
* permission to link this software with independent modules, and to copy and
* distribute the resulting executable under terms of your choice, provided that
* you also meet, for each linked independent module, the terms and conditions of
* the license of that module.  An independent module is a module which is not
* derived from this software.  The special exception does not apply to any
* modifications of the software.
* 
*      Notwithstanding the above, under no circumstances may you combine this
* software in any way with any other Broadcom software provided under a license
* other than the GPL, without Broadcom's express prior written consent.
*
* $Id: dhd_custom_gpio.c 441177 2013-12-05 07:03:52Z $
*/

#include <typedefs.h>
#include <linuxver.h>
#include <osl.h>
#include <bcmutils.h>

#include <dngl_stats.h>
#include <dhd.h>

#include <wlioctl.h>
#include <wl_iw.h>

#define WL_ERROR(x) printf x
#define WL_TRACE(x)

#ifdef CUSTOMER_HW
extern  void bcm_wlan_power_off(int);
extern  void bcm_wlan_power_on(int);
#endif /* CUSTOMER_HW */
#if defined(CUSTOMER_HW2)

#if defined(PLATFORM_MPS)
int __attribute__ ((weak)) wifi_get_fw_nv_path(char *fw, char *nv) { return 0;};
#endif

#ifdef CONFIG_WIFI_CONTROL_FUNC
int wifi_set_power(int on, unsigned long msec);
int wifi_get_irq_number(unsigned long *irq_flags_ptr);
int wifi_get_mac_addr(unsigned char *buf);
void *wifi_get_country_code(char *ccode);
#else
int wifi_set_power(int on, unsigned long msec) { return -1; }
int wifi_get_irq_number(unsigned long *irq_flags_ptr) { return -1; }
int wifi_get_mac_addr(unsigned char *buf) { return -1; }
void *wifi_get_country_code(char *ccode) { return NULL; }
#endif /* CONFIG_WIFI_CONTROL_FUNC */
#endif 

#ifdef GET_CUSTOM_MAC_ENABLE
#define MACADDR_BUF_LEN 64
#define MACADDR_PATH "/data/misc/wifi/.mac.info"
#endif /* GET_CUSTOM_MAC_ENABLE */

#if defined(OOB_INTR_ONLY)

#if defined(BCMLXSDMMC)
extern int sdioh_mmc_irq(int irq);
#endif /* (BCMLXSDMMC)  */

#if defined(CUSTOMER_HW3) || defined(PLATFORM_MPS)
#include <mach/gpio.h>
#endif

/* Customer specific Host GPIO defintion  */
static int dhd_oob_gpio_num = -1;

module_param(dhd_oob_gpio_num, int, 0644);
MODULE_PARM_DESC(dhd_oob_gpio_num, "DHD oob gpio number");

/* This function will return:
 *  1) return :  Host gpio interrupt number per customer platform
 *  2) irq_flags_ptr : Type of Host interrupt as Level or Edge
 *
 *  NOTE :
 *  Customer should check his platform definitions
 *  and his Host Interrupt spec
 *  to figure out the proper setting for his platform.
 *  Broadcom provides just reference settings as example.
 *
 */
int dhd_customer_oob_irq_map(unsigned long *irq_flags_ptr)
{
	int  host_oob_irq = 0;

#if defined(CUSTOMER_HW2)
	host_oob_irq = wifi_get_irq_number(irq_flags_ptr);

#else
#if defined(CUSTOM_OOB_GPIO_NUM)
	if (dhd_oob_gpio_num < 0) {
		dhd_oob_gpio_num = CUSTOM_OOB_GPIO_NUM;
	}
#endif /* CUSTOMER_OOB_GPIO_NUM */

	if (dhd_oob_gpio_num < 0) {
		WL_ERROR(("%s: ERROR customer specific Host GPIO is NOT defined \n",
		__FUNCTION__));
		return (dhd_oob_gpio_num);
	}

	WL_ERROR(("%s: customer specific Host GPIO number is (%d)\n",
	         __FUNCTION__, dhd_oob_gpio_num));

#if defined CUSTOMER_HW
	host_oob_irq = MSM_GPIO_TO_INT(dhd_oob_gpio_num);
#elif defined CUSTOMER_HW3
	gpio_request(dhd_oob_gpio_num, "oob irq");
	host_oob_irq = gpio_to_irq(dhd_oob_gpio_num);
	gpio_direction_input(dhd_oob_gpio_num);
#endif /* CUSTOMER_HW */
#endif 

	return (host_oob_irq);
}
#endif 

/* Customer function to control hw specific wlan gpios */
void
dhd_customer_gpio_wlan_ctrl(int onoff)
{
	switch (onoff) {
		case WLAN_RESET_OFF:
			WL_TRACE(("%s: call customer specific GPIO to insert WLAN RESET\n",
				__FUNCTION__));
#ifdef CUSTOMER_HW
			bcm_wlan_power_off(2);
#endif /* CUSTOMER_HW */
#if defined(CUSTOMER_HW2)
			wifi_set_power(0, WIFI_TURNOFF_DELAY);
#endif
			WL_ERROR(("=========== WLAN placed in RESET ========\n"));
		break;

		case WLAN_RESET_ON:
			WL_TRACE(("%s: callc customer specific GPIO to remove WLAN RESET\n",
				__FUNCTION__));
#ifdef CUSTOMER_HW
			bcm_wlan_power_on(2);
#endif /* CUSTOMER_HW */
#if defined(CUSTOMER_HW2)
			wifi_set_power(1, WIFI_TURNON_DELAY);
#endif
			WL_ERROR(("=========== WLAN going back to live  ========\n"));
		break;

		case WLAN_POWER_OFF:
			WL_TRACE(("%s: call customer specific GPIO to turn off WL_REG_ON\n",
				__FUNCTION__));
#ifdef CUSTOMER_HW
			bcm_wlan_power_off(1);
#endif /* CUSTOMER_HW */
		break;

		case WLAN_POWER_ON:
			WL_TRACE(("%s: call customer specific GPIO to turn on WL_REG_ON\n",
				__FUNCTION__));
#ifdef CUSTOMER_HW
			bcm_wlan_power_on(1);
			/* Lets customer power to get stable */
			OSL_DELAY(200);
#endif /* CUSTOMER_HW */
		break;
	}
}

#ifdef GET_CUSTOM_MAC_ENABLE
int somc_get_mac_address(unsigned char *buf)
{
	int ret = -EINVAL;
	int len;
	unsigned char macaddr_buf[MACADDR_BUF_LEN];
	void *fp = NULL;
	struct ether_addr eth;

	if (!buf)
		return -EINVAL;

	fp = dhd_os_open_image(MACADDR_PATH);
	if (!fp) {
		WL_ERROR(("%s: file open error\n", __FUNCTION__));
		goto err;
	}

	len = dhd_os_get_image_block(macaddr_buf, MACADDR_BUF_LEN, fp);
	if (len <= 0 || MACADDR_BUF_LEN <= len) {
		WL_ERROR(("%s: file read error\n", __FUNCTION__));
		goto err;
	}
	macaddr_buf[len] = '\0';

	/* convert mac address */
	ret = !bcm_ether_atoe(macaddr_buf, &eth);
	if (ret) {
		WL_ERROR(("%s: convert mac value fail\n", __FUNCTION__));
		goto err;
	}

	memcpy(buf, eth.octet, ETHER_ADDR_LEN);
err:
	if (fp)
		dhd_os_close_image(fp);
	return ret;
}
#endif

#ifdef GET_CUSTOM_MAC_ENABLE
/* Function to get custom MAC address */
int
dhd_custom_get_mac_address(unsigned char *buf)
{
	int ret = 0;

	WL_TRACE(("%s Enter\n", __FUNCTION__));
	if (!buf)
		return -EINVAL;

	/* Customer access to MAC address stored outside of DHD driver */
#if (defined(CUSTOMER_HW2) || defined(CUSTOMER_HW10)) && (LINUX_VERSION_CODE >= \
	KERNEL_VERSION(2, 6, 35))
	ret = somc_get_mac_address(buf);
#endif

#ifdef EXAMPLE_GET_MAC
	/* EXAMPLE code */
	{
		struct ether_addr ea_example = {{0x00, 0x11, 0x22, 0x33, 0x44, 0xFF}};
		bcopy((char *)&ea_example, buf, sizeof(struct ether_addr));
	}
#endif /* EXAMPLE_GET_MAC */

	return ret;
}
#endif /* GET_CUSTOM_MAC_ENABLE */

/* Customized Locale table : OPTIONAL feature */
const struct cntry_locales_custom translate_custom_table[] = {
/* Table should be filled out based on custom platform regulatory requirement */
#ifdef EXAMPLE_TABLE
                {"AR", "AR", 1},
                {"AT", "AT", 1},
                {"AU", "AU", 2},
                {"BE", "BE", 1},
                {"BG", "BG", 1},
                {"BN", "BN", 1},
                {"CA", "CA", 2},
                {"CH", "CH", 1},
                {"CY", "CY", 1},
                {"CZ", "CZ", 1},
                {"DE", "DE", 3},
                {"DK", "DK", 1},
                {"EE", "EE", 1},
                {"ES", "ES", 1},
                {"FI", "FI", 1},
                {"FR", "FR", 1},
                {"GB", "GB", 1},
                {"GR", "GR", 1},
                {"HR", "HR", 1},
                {"HU", "HU", 1},
                {"IE", "IE", 1},
                {"IS", "IS", 1},
                {"IT", "IT", 1},
                {"JP", "JP", 5},
                {"KR", "KR", 24},
                {"KW", "KW", 1},
                {"LI", "LI", 1},
                {"LT", "LT", 1},
                {"LU", "LU", 1},
                {"LV", "LV", 1},
                {"MT", "MT", 1},
                {"NL", "NL", 1},
                {"NO", "NO", 1},
                {"PL", "PL", 1},
                {"PT", "PT", 1},
                {"PY", "PY", 1},
                {"RO", "RO", 1},
                {"RU", "RU", 13},
                {"SE", "SE", 1},
                {"SI", "SI", 1},
                {"SK", "SK", 1},
                {"TW", "TW", 2},
                {"",   "XZ", 11}, /* Universal if Country code is unknown or empty */
                {"IR", "XZ", 11}, /* Universal if Country code is IRAN, (ISLAMIC REPUBLIC OF) */
                {"SD", "XZ", 11}, /* Universal if Country code is SUDAN */
                {"SY", "XZ", 11}, /* Universal if Country code is SYRIAN ARAB REPUBLIC */
                {"GL", "XZ", 11}, /* Universal if Country code is GREENLAND */
                {"PS", "XZ", 11}, /* Universal if Country code is PALESTINIAN TERRITORY, OCCUPIED */
                {"TL", "XZ", 11}, /* Universal if Country code is TIMOR-LESTE (EAST TIMOR) */
                {"MH", "XZ", 11}, /* Universal if Country code is MARSHALL ISLANDS */
                {"US", "US", 109},
                {"UA", "UA", 8},
                {"CO", "CO", 4},
                {"ID", "ID", 1},
                {"LA", "LA", 1},
                {"LB", "LB", 2},
                {"VN", "VN", 4},
                {"MA", "MA", 1},
                {"TR", "TR", 7},
                {"AE", "AE", 1},
                {"MX", "MX", 1},
                {"CN", "CN", 11},
#endif /* EXMAPLE_TABLE */
};


/* Customized Locale convertor
*  input : ISO 3166-1 country abbreviation
*  output: customized cspec
*/
void get_customized_country_code(char *country_iso_code, wl_country_t *cspec)
{
#if defined(CUSTOMER_HW2) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39))

	struct cntry_locales_custom *cloc_ptr;

	if (!cspec)
		return;

	cloc_ptr = wifi_get_country_code(country_iso_code);
	if (cloc_ptr) {
		strlcpy(cspec->ccode, cloc_ptr->custom_locale, WLC_CNTRY_BUF_SZ);
		cspec->rev = cloc_ptr->custom_locale_rev;
	}
	return;
#else
	int size, i;

	size = ARRAYSIZE(translate_custom_table);

	if (cspec == 0)
		 return;

	if (size == 0)
		 return;

	for (i = 0; i < size; i++) {
		if (strcmp(country_iso_code, translate_custom_table[i].iso_abbrev) == 0) {
			memcpy(cspec->ccode,
				translate_custom_table[i].custom_locale, WLC_CNTRY_BUF_SZ);
			cspec->rev = translate_custom_table[i].custom_locale_rev;
			return;
		}
	}
#ifdef EXAMPLE_TABLE
	/* if no country code matched return first universal code from translate_custom_table */
	memcpy(cspec->ccode, translate_custom_table[0].custom_locale, WLC_CNTRY_BUF_SZ);
	cspec->rev = translate_custom_table[0].custom_locale_rev;
#endif /* EXMAPLE_TABLE */
	return;
#endif /* defined(CUSTOMER_HW2) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)) */
}
