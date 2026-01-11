.PHONY: all clean install uninstall

PLUGINS = flowchart syntax canvas storage

all: $(PLUGINS)

$(PLUGINS):
	$(MAKE) -C $@

clean:
	for plugin in $(PLUGINS); do \
		$(MAKE) -C $$plugin clean; \
	done

install: all
	for plugin in $(PLUGINS); do \
		$(MAKE) -C $$plugin install; \
	done

uninstall:
	for plugin in $(PLUGINS); do \
		$(MAKE) -C $$plugin uninstall || true; \
	done
