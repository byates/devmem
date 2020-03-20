################################################################################
#
# devmem
#
################################################################################

DEVMEM_VERSION = 1.0.0
DEVMEM_SITE_METHOD = wget
DEVMEM_SOURCE = $(DEVMEM_VERSION).tar.gz
DEVMEM_SITE = https://github.com/byates/devmem/archive
DEVMEM_LICENSE = GPL-2.0
DEVMEM_INSTALL_STAGING = NO
DEVMEM_INSTALL_TARGET = YES

define DEVMEM_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)
endef

# Files that may exists on target and SDK. Typically all libraries (static and
# shared), all config files, etc.
define DEVMEM_INSTALL_STAGING_CMDS
endef

# Files that only need to be on the target. Compared to staging/,
# target/ contains only the files and libraries needed to run the
# selected target applications: the development files (headers,
# etc.) are not present, the binaries are stripped.
define DEVMEM_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/devmem $(TARGET_DIR)/usr/bin/devmem
endef

$(eval $(generic-package))
