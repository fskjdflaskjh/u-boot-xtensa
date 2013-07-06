#
# SPDX-License-Identifier:	GPL-2.0+
#

dtb-$(CONFIG_XTFPGA) += kc705.dtb

targets += $(dtb-y)

DTC_FLAGS +=

PHONY += dtbs
dtbs: $(addprefix $(obj)/, $(dtb-y))
	@:

clean-files := *.dtb
