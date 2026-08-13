from org.csstudio.display.builder.runtime.script import PVUtil, ScriptUtil
from java.util.logging import Logger

# https://github.com/ControlSystemStudio/phoebus/blob/master/app/display/runtime/src/main/java/org/csstudio/display/builder/runtime/script/PVUtil.java

logger = Logger.getLogger('name')
print("PVUtil has the methods:\n\n", dir(PVUtil))

widget = locals()['widget']
pvs = locals()['pvs']

selected_event_index_pv = pvs[0]
display_selected_event_index = pvs[1]
selected_event_index = PVUtil.getLong(selected_event_index_pv)
display_selected_event_index.setValue(selected_event_index)
logger.info('set value to {}'.format(selected_event_index))

event_array = PVUtil.getLongArray(pvs[2])

selected_event = pvs[3]
selected_event.setValue(event_array[selected_event_index])
