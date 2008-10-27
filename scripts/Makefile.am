EXTRA_DIST = startup.sl __main__.py toc.py joo_utils.py rot13.py dcc.py persistant.py pastebin.py

scriptdir = $(datadir)/$(PACKAGE)-$(VERSION)/scripts

script_DATA = rot13.py startup.sl __main__.py toc.py joo_utils.py dcc.py persistant.py pastebin.py

% : $(top_srcdir)/%.tmplt
	cat $(top_srcdir)/$< | sed -e "s:@package@:$(PACKAGE):g" | sed -e "s:@version@:$(VERSION):g" > $@
