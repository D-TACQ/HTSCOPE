TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS BELOW HERE

# Install databases, templates & substitutions like this
#DB += circle.db
#DB += dbExample1.db
#DB += dbExample2.db
DB += xrmSliceVersion.db
#DB += dbSubExample.db
#DB += user.substitutions
DB +=  xrmSliceAI_HT_32CH.db
xrmSliceAI_HT_32CH_TEMPLATE = xrmSliceAI_HT.template
xrmSliceAI_HT_32CH_SUBSTITUTIONS = xrmSliceAI_HT_32CH.substitutions
DB +=  xrmSliceAI_HT_16CH.db
xrmSliceAI_HT_16CH_TEMPLATE = xrmSliceAI_HT.template
xrmSliceAI_HT_16CH_SUBSTITUTIONS = xrmSliceAI_HT_16CH.substitutions


DB +=  xrmSliceAI_PM_32CH.db
xrmSliceAI_PM_32CH_TEMPLATE = xrmSliceAI_PM.template
xrmSliceAI_PM_32CH_SUBSTITUTIONS = xrmSliceAI_PM_32CH.substitutions
DB +=  xrmSliceAI_PM_16CH.db
xrmSliceAI_PM_16CH_TEMPLATE = xrmSliceAI_PM.template
xrmSliceAI_PM_16CH_SUBSTITUTIONS = xrmSliceAI_PM_16CH.substitutions


# If <anyname>.db template is not named <anyname>*.template add
# <anyname>_TEMPLATE = <templatename>

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD EXTRA GNUMAKE RULES BELOW HERE

