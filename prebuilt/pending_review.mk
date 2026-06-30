# MK Pending Review — Facts awaiting verification
# Format: source|relation|target|confidence|status|reason|sourceName|timestamp
# These facts passed some gates but need confirmation before entering main graph.
# 
# Facts end up here when:
#   - Confidence is between 0.2 and 0.5 (not enough to auto-approve)
#   - A trusted source contradicts existing knowledge (needs manual review)
#   - Logical consistency check failed but source has high trust
#
# To promote a fact: use MKKnowledgeGate::promote(factId) or ::promoteByContent()
# Promoted facts move to the main knowledge graph (learned_facts.mk)
#
# This file is auto-managed by knowledge_gate.cpp — do not edit manually.

