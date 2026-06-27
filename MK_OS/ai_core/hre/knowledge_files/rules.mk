# ============================================================
# MK REASONING RULES - Comprehensive Inference Engine
# ============================================================
# Format: rule_name|condition1_sub condition1_rel condition1_obj|condition2_sub condition2_rel condition2_obj|conclusion_sub conclusion_rel conclusion_obj|confidence
#
# These rules allow MK to derive new facts from existing knowledge
# by applying logical inference patterns.
# ============================================================

# --- INHERITANCE RULES ---
# If X is_a Y and Y has Z, then X has Z
inheritance_has|?x is_a ?y|?y has ?z|?x has ?z|0.85
# If X is_a Y and Y has_property Z, then X has_property Z
inheritance_property|?x is_a ?y|?y has_property ?z|?x has_property ?z|0.85
# If X is_a Y and Y can Z, then X can Z
inheritance_ability|?x is_a ?y|?y can ?z|?x can ?z|0.80
# If X is_a Y and Y requires Z, then X requires Z
inheritance_requirement|?x is_a ?y|?y requires ?z|?x requires ?z|0.80
# If X is_a Y and Y belongs_to Z, then X belongs_to Z
inheritance_membership|?x is_a ?y|?y belongs_to ?z|?x belongs_to ?z|0.85
# If X instance_of Y and Y is_a Z, then X instance_of Z
inheritance_chain|?x instance_of ?y|?y is_a ?z|?x instance_of ?z|0.80

# --- TRANSITIVITY RULES ---
# Location transitivity: if X located_in Y and Y located_in Z, then X located_in Z
location_transitive|?x located_in ?y|?y located_in ?z|?x located_in ?z|0.90
# Ownership transitivity: if X owns Y and Y contains Z, then X owns Z
ownership_transitive|?x owns ?y|?y contains ?z|?x owns ?z|0.70
# Categorization transitivity: if X part_of Y and Y part_of Z, then X part_of Z
category_transitive|?x part_of ?y|?y part_of ?z|?x part_of ?z|0.85
# If X subset_of Y and Y subset_of Z, then X subset_of Z
subset_transitive|?x subset_of ?y|?y subset_of ?z|?x subset_of ?z|0.90
# If X faster_than Y and Y faster_than Z, then X faster_than Z
speed_transitive|?x faster_than ?y|?y faster_than ?z|?x faster_than ?z|0.90
# If X larger_than Y and Y larger_than Z, then X larger_than Z
size_transitive|?x larger_than ?y|?y larger_than ?z|?x larger_than ?z|0.90
# If X older_than Y and Y older_than Z, then X older_than Z
age_transitive|?x older_than ?y|?y older_than ?z|?x older_than ?z|0.90
# If X depends_on Y and Y depends_on Z, then X indirectly_depends_on Z
dependency_transitive|?x depends_on ?y|?y depends_on ?z|?x indirectly_depends_on ?z|0.75

# --- SYMMETRY RULES ---
# If X friend_of Y, then Y friend_of X
symmetry_friend|?x friend_of ?y|?y exists_as entity|?y friend_of ?x|0.95
# If X opposite_of Y, then Y opposite_of X
symmetry_opposite|?x opposite_of ?y|?y exists_as entity|?y opposite_of ?x|0.99
# If X sibling_of Y, then Y sibling_of X
symmetry_sibling|?x sibling_of ?y|?y exists_as entity|?y sibling_of ?x|0.99
# If X married_to Y, then Y married_to X
symmetry_married|?x married_to ?y|?y exists_as entity|?y married_to ?x|0.99
# If X similar_to Y, then Y similar_to X
symmetry_similar|?x similar_to ?y|?y exists_as entity|?y similar_to ?x|0.95
# If X connected_to Y, then Y connected_to X
symmetry_connected|?x connected_to ?y|?y exists_as entity|?y connected_to ?x|0.90
# If X collaborates_with Y, then Y collaborates_with X
symmetry_collab|?x collaborates_with ?y|?y exists_as entity|?y collaborates_with ?x|0.95

# --- CAUSALITY RULES ---
# If X causes Y and Y causes Z, then X indirectly_causes Z
causality_chain|?x causes ?y|?y causes ?z|?x indirectly_causes ?z|0.70
# If X prevents Y and Y causes Z, then X indirectly_prevents Z
prevention_chain|?x prevents ?y|?y causes ?z|?x indirectly_prevents ?z|0.65
# If X enables Y and Y leads_to Z, then X contributes_to Z
enablement_chain|?x enables ?y|?y leads_to ?z|?x contributes_to ?z|0.60
# If X requires Y and Y requires Z, then X indirectly_requires Z
requirement_chain|?x requires ?y|?y requires ?z|?x indirectly_requires ?z|0.75
# If X improves Y and Y part_of Z, then X benefits Z
improvement_propagation|?x improves ?y|?y part_of ?z|?x benefits ?z|0.65

# --- NEGATION RULES ---
# If X opposite_of Y and Y has Z, then X lacks Z
negation_opposite|?x opposite_of ?y|?y has ?z|?x lacks ?z|0.75
# If X opposite_of Y and Y causes Z, then X prevents Z
negation_causality|?x opposite_of ?y|?y causes ?z|?x prevents ?z|0.70
# If X contradicts Y and Y is_a Z, then X not_a Z
negation_category|?x contradicts ?y|?y is_a ?z|?x not_a ?z|0.65
# If X incompatible_with Y and Y requires Z, then X conflicts_with Z
negation_compatibility|?x incompatible_with ?y|?y requires ?z|?x conflicts_with ?z|0.60

# --- DOMAIN INFERENCE RULES ---
# If X uses language Y, then X is_a programmer
domain_programmer|?x uses ?y|?y is_a programming_language|?x is_a programmer|0.70
# If X studies Y and Y is_a science, then X is_a scientist
domain_scientist|?x studies ?y|?y is_a science|?x is_a scientist|0.65
# If X writes Y and Y is_a book, then X is_a author
domain_author|?x writes ?y|?y is_a book|?x is_a author|0.75
# If X plays Y and Y is_a instrument, then X is_a musician
domain_musician|?x plays ?y|?y is_a instrument|?x is_a musician|0.70
# If X teaches Y and Y is_a subject, then X is_a teacher
domain_teacher|?x teaches ?y|?y is_a subject|?x is_a teacher|0.75
# If X researches Y and Y is_a field, then X expert_in Y
domain_expert|?x researches ?y|?y is_a field|?x expert_in ?y|0.60

# --- COMPOSITION RULES ---
# If X made_of Y and Y has_property Z, then X has_property Z
material_property|?x made_of ?y|?y has_property ?z|?x has_property ?z|0.75
# If X contains Y and Y is_a Z, then X has Z
containment_type|?x contains ?y|?y is_a ?z|?x has ?z|0.70
# If X built_with Y and Y good_for Z, then X good_for Z
tool_purpose|?x built_with ?y|?y good_for ?z|?x good_for ?z|0.65

# --- PURPOSE AND FUNCTION RULES ---
# If X is_a Y and Y used_for Z, then X used_for Z
purpose_inherit|?x is_a ?y|?y used_for ?z|?x used_for ?z|0.80
# If X designed_for Y and Y requires Z, then X supports Z
design_requirement|?x designed_for ?y|?y requires ?z|?x supports ?z|0.70
# If X replaces Y and Y used_for Z, then X used_for Z
replacement_purpose|?x replaces ?y|?y used_for ?z|?x used_for ?z|0.75

# --- TEMPORAL RULES ---
# If X precedes Y and Y precedes Z, then X precedes Z
temporal_transitive|?x precedes ?y|?y precedes ?z|?x precedes ?z|0.90
# If X invented_before Y and Y improved_by Z, then X ancestor_of Z
temporal_ancestry|?x invented_before ?y|?y improved_by ?z|?x ancestor_of ?z|0.60
# If X founded_in Y and Y part_of Z, then X active_during Z
temporal_period|?x founded_in ?y|?y part_of ?z|?x active_during ?z|0.70

# --- SPATIAL RULES ---
# If X near Y and Y near Z, then X possibly_near Z
spatial_proximity|?x near ?y|?y near ?z|?x possibly_near ?z|0.50
# If X capital_of Y and Y located_in Z, then X located_in Z
spatial_capital|?x capital_of ?y|?y located_in ?z|?x located_in ?z|0.95
# If X border Y and Y has_climate Z, then X has_similar_climate Z
spatial_climate|?x borders ?y|?y has_climate ?z|?x has_similar_climate ?z|0.45
