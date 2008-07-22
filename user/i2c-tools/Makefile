VER = i2c-tools-3.0.1

all:
	$(MAKE) -C $(VER)
	$(MAKE) -C $(VER) DESTDIR=$(STAGEDIR) install

clean:
	$(MAKE) -C $(VER) clean

romfs:
	$(ROMFSINST) -d -e CONFIG_USER_I2CTOOLS_I2CTOOLS $(STAGEDIR)/usr/sbin/i2cdetect /usr/sbin/i2cdetect
	$(ROMFSINST) -d -e CONFIG_USER_I2CTOOLS_I2CTOOLS $(STAGEDIR)/usr/sbin/i2cdump /usr/sbin/i2cdump
	$(ROMFSINST) -d -e CONFIG_USER_I2CTOOLS_I2CTOOLS $(STAGEDIR)/usr/sbin/i2cget /usr/sbin/i2cget
	$(ROMFSINST) -d -e CONFIG_USER_I2CTOOLS_I2CTOOLS $(STAGEDIR)/usr/sbin/i2cset /usr/sbin/i2cset

.PHONY: all clean romfs
