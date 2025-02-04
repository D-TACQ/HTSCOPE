import time
from java.util.logging import Logger
from org.csstudio.display.builder.runtime.script import PVUtil
from org.phoebus.framework.macros import Macros

#Startup
logger = Logger.getLogger('init_controls')
widget = locals()['widget']
pvs = locals()['pvs']
display = widget.topDisplayModel

#functions

def get_macros(widget):
	macros = widget.getPropertyValue("macros")
	extracted = {}
	for key in macros.getNames():
		extracted[key] = macros.getValue(key)
	return extracted

def set_macros(widget, new_macros):
    macros = Macros()
    for key in new_macros:
        macros.add(key, new_macros[key])
    widget.setPropertyValue("macros", macros)
    return macros

logger.info("Start")

uuts = PVUtil.getString(pvs[0]).split(',')

macros = get_macros(display)

for idx in range(4):
    key = "UUT_{}".format(idx + 1)
    macros[key] = uuts[idx] if 0 <= idx < len(uuts) else None

widget.setPropertyValue("macros", macros)
widget.setPropertyValue("file", "opi/ht_scope_controls.bob")
logger.info("Complete")