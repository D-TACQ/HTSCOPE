from org.csstudio.display.builder.model.properties import WidgetColor
from org.csstudio.display.builder.runtime.script import PVUtil
from java.util.logging import Logger


"""Set the widget color on cstate change"""

# Startup
logger = Logger.getLogger('handle_cstate')
widget = locals()['widget']
pvs = locals()['pvs']
display = widget.topDisplayModel

# Main

value = PVUtil.getInt(pvs[0])

if value == 0:
    color = WidgetColor(255, 0, 0) #Red
elif value == 1:
    color = WidgetColor(255, 128, 0) #Orange
elif 2 <= value <= 3:
    color = WidgetColor(0, 255, 0) #Green
elif 4 <= value <= 5:
    color = WidgetColor(139, 105, 20) #Brown

widget.setPropertyValue('background_color', color)
