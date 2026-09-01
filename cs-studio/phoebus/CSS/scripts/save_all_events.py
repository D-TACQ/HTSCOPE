from org.csstudio.display.builder.runtime.script import PVUtil, ScriptUtil
from java.util.logging import Logger
from time import sleep

# https://github.com/ControlSystemStudio/phoebus/blob/master/app/display/runtime/src/main/java/org/csstudio/display/builder/runtime/script/PVUtil.java

logger = Logger.getLogger('save_all_events')
print("PVUtil has the methods:\n\n", dir(PVUtil))

widget = locals()['widget']
pvs = locals()['pvs']

event_index_uut1_pv = pvs[0]
save_event_uut1_pv = pvs[1]
save_all_events_button_stored_pv = pvs[3]

event_uut1_array = PVUtil.getLongArray(event_index_uut1_pv)
event_uut2_array = PVUtil.getLongArray(event_index_uut2_pv)


if PVUtil.getLong(save_all_events_button_stored_pv) == 0:
    logger.info('stored pv is zero, not executing')
else:
    for event in range(0, len(event_uut1_array)):
        selected_event = event_uut1_array[event]
        if selected_event != 0:
            logger.info('going to sleep')
            sleep(0.5)
            logger.info('awake')
            save_event_uut1_pv.setValue(event)
            logger.info('save event {}'.format(event))
            logger.info('save event at byte index {}'.format(selected_event))

    save_all_events_button_stored_pv.setValue(0)
