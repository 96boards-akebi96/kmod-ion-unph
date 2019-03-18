include Kbuild

modules_install: modules
modules install modules_install headers_install clean distclean: $(obj-y)
	@for i in $(subst modules,,$^) ; do \
		$(MAKE) -C $${i} $@; \
		if [ 0 -ne $$? ]; then exit 1; fi; \
	done

.PHONY: modules install modules_install headers_install \
	clean distclean $(obj-y)
