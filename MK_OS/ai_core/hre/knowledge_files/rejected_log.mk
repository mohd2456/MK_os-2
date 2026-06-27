# MK Rejected Facts Log — Facts that failed validation
# Format: source|relation|target|confidence|REJECTED|reason|sourceName|timestamp
#
# Facts end up here when:
#   - Confidence is below 0.2 (extremely unreliable)
#   - Contradiction detected with existing verified knowledge
#   - Logical consistency check failed AND source has low trust
#   - Source trust is below 0.2 and no corroborating sources exist
#
# Rejection reasons include:
#   - "Exclusive relation conflict" — contradicts a known unique fact
#   - "Opposite category conflict" — claims something is in incompatible categories
#   - "Circular reference" — would create a logical loop
#   - "Domain mismatch" — concept types are incompatible
#   - "Extremely low confidence" — no supporting evidence
#
# This file is auto-managed by knowledge_gate.cpp — do not edit manually.
# Historical record: rejected facts are NEVER deleted, only appended.

