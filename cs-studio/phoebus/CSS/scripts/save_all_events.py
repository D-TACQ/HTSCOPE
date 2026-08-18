from org.csstudio.display.builder.runtime.script import PVUtil, ScriptUtil
from java.util.logging import Logger

# https://github.com/ControlSystemStudio/phoebus/blob/master/app/display/runtime/src/main/java/org/csstudio/display/builder/runtime/script/PVUtil.java

logger = Logger.getLogger('name')
print("PVUtil has the methods:\n\n", dir(PVUtil))

widget = locals()['widget']
pvs = locals()['pvs']

event_index_pv = pvs[0]
save_event_pv = pvs[1]


event_array = PVUtil.getLongArray(pvs[0])


for event in range(0, len(event_array)):
    selected_event = event_array[event]
    if selected_event != 0:
        save_event_pv.setValue(event)
        logger.info('save event {}'.format(event))
        logger.info('save event at byte index {}'.format(selected_event))
