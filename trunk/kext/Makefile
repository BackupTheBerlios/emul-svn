all:
install:
	@if test "`uname`" = "Darwin"; then \
	if test ! -e /System/Library/Extensions/EMUL.kext; then \
	cp -r EMUL.kext /System/Library/Extensions/ ;\
	touch /System/Library/Extensions ;\
	echo ""; echo "   emul kernel extension installed" ;\
	echo ""; echo "   If this is your first install, please reboot the system." ;\
	echo "----------------------------------------------------------------------" ; fi \
	fi

uninstall:
	@if test "`uname`" = "Darwin"; then \
	if test -e /System/Library/Extensions/EMUL.kext; then \
	rm -rf /System/Library/Extensions/EMUL.kext ;\
	echo ""; echo "   EMUL.kext kernel extension removed"; echo ""; fi \
	fi
clean:
distclean:
