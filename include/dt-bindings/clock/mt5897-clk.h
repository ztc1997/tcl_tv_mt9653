/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <dt-bindings/clock/mt5897_ckgen00_pm_reg.h>
#include <dt-bindings/clock/mt5897_ckgen00_reg.h>
#include <dt-bindings/clock/mt5897_ckgen01_pm_reg.h>
#include <dt-bindings/clock/mt5897_ckgen01_reg.h>

/* CLK ID define */
#include <dt-bindings/clock/merak/adcpll-clk.h>
#include <dt-bindings/clock/merak/audioEn_clk.h>
#include <dt-bindings/clock/merak/top_clk.h>
#include <dt-bindings/clock/merak/fcieEn_clk.h>
#include <dt-bindings/clock/merak/geEn_clk.h>
#include <dt-bindings/clock/merak/hdmi-clk.h>
#include <dt-bindings/clock/merak/hdmiEn_clk.h>
#include <dt-bindings/clock/merak/i2c_En_clk.h>
#include <dt-bindings/clock/merak/jpd-clk.h>
#include <dt-bindings/clock/merak/mheg5-clk.h>
#include <dt-bindings/clock/merak/nonpm_adcEn_clk.h>
#include <dt-bindings/clock/merak/pm_adcEn_clk.h>
#include <dt-bindings/clock/merak/pm-clk.h>
#include <dt-bindings/clock/merak/pwmen_clk.h>
#include <dt-bindings/clock/merak/scace-clk.h>
#include <dt-bindings/clock/merak/scdv-clk.h>
#include <dt-bindings/clock/merak/scip-clk.h>
#include <dt-bindings/clock/merak/scmw-clk.h>
#include <dt-bindings/clock/merak/scnr-clk.h>
#include <dt-bindings/clock/merak/scpq-clk.h>
#include <dt-bindings/clock/merak/scscl-clk.h>
#include <dt-bindings/clock/merak/scsrc-clk.h>
#include <dt-bindings/clock/merak/scsr-clk.h>
#include <dt-bindings/clock/merak/sctcon-clk.h>
#include <dt-bindings/clock/merak/smartEn_clk.h>
#include <dt-bindings/clock/merak/tsp-clk_fix.h>
#include <dt-bindings/clock/merak/tsp-clk.h>
#include <dt-bindings/clock/merak/tspEn_clk.h>
#include <dt-bindings/clock/merak/uartEn_clk.h>
#include <dt-bindings/clock/merak/ufsEn_clk.h>
#include <dt-bindings/clock/merak/usb_nonpmEn_clk.h>
#include <dt-bindings/clock/merak/usb_pmEn_clk.h>
#include <dt-bindings/clock/merak/vdEn_clk.h>
#include <dt-bindings/clock/merak/vivaldi9-clk_fix.h>
#include <dt-bindings/clock/merak/vivaldi9-clk.h>
#include <dt-bindings/clock/merak/adc_fix.h>
#include <dt-bindings/clock/merak/pm_fix.h>
#include <dt-bindings/clock/merak/b2rEn_clk.h>
#include <dt-bindings/clock/merak/cvbso_adc_clk.h>

/* mt5897 info */
#include <dt-bindings/clock/mt5897/top_clk.h>
#include <dt-bindings/clock/mt5897/usb.h>
#include <dt-bindings/clock/mt5897/tspEn_clk.h>
#include <dt-bindings/clock/mt5897/audioEn.h>
#include <dt-bindings/clock/mt5897/aud.h>
#include <dt-bindings/clock/mt5897/vdec_clk.h>
#include <dt-bindings/clock/mt5897/vdecEn_clk.h>
#include <dt-bindings/clock/mt5897/jpd-clk.h>
#include <dt-bindings/clock/mt5897/frccore1-clk.h>
#include <dt-bindings/clock/mt5897/frccore3-clk.h>
#include <dt-bindings/clock/mt5897/scace-clk.h>
#include <dt-bindings/clock/mt5897/scdisp-clk.h>
#include <dt-bindings/clock/mt5897/scdv-clk.h>
#include <dt-bindings/clock/mt5897/scip-clk.h>
#include <dt-bindings/clock/mt5897/scmw-clk.h>
#include <dt-bindings/clock/mt5897/scpq-clk.h>
#include <dt-bindings/clock/mt5897/scscl-clk.h>
#include <dt-bindings/clock/mt5897/sctcon-clk.h>
#include <dt-bindings/clock/mt5897/xcEn_clk.h>
#include <dt-bindings/clock/mt5897/xcgate-clk.h>
#include <dt-bindings/clock/mt5897/venc_clk.h>
#include <dt-bindings/clock/mt5897/earc-clk_special.h>
#include <dt-bindings/clock/mt5897/earcEn_clk.h>
#include <dt-bindings/clock/mt5897/miic-clk.h>
#include <dt-bindings/clock/mt5897/fuart-clk.h>
#include <dt-bindings/clock/mt5897/uart-clk.h>
#include <dt-bindings/clock/mt5897/uart-en.h>
