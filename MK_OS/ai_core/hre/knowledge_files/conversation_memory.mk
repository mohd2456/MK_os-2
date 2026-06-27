# MK Cross-Session Conversation Memory
# Format: source|relation|target|weight
#
# This file stores important context from previous conversations so MK
# can remember what was discussed across sessions.
#
# Types of entries:
#   session_topic|was_about|<topic>|1.0     — Records what topic was discussed
#   user|preference|<preference>|1.0        — Records user preferences detected
#   conversation|learned|<fact>|0.8         — New facts learned during conversation
#   user|asked_about|<topic>|1.0            — Topics the user has asked about
#   user|interested_in|<topic>|0.9          — Inferred user interests
#
# Managed by conversation_engine.cpp — saveSessionMemory()
# Loaded at startup by conversation_engine.cpp — loadSessionMemory()

