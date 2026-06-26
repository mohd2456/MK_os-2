# MK Custom Reasoning Rules
# Format: name|cond1_sub cond1_rel cond1_obj|cond2_sub cond2_rel cond2_obj|conc_sub conc_rel conc_obj|confidence
# These rules extend the built-in reasoning chains with additional inference patterns.

# If X eats Y and Y is_a Z, then X eats Z (dietary generalization)
diet_generalize|?x eats ?y|?y is_a ?z|?x eats ?z|0.7

# If X made_of Y and Y has_property Z, then X has_property Z
material_property|?x made_of ?y|?y has_property ?z|?x has_property ?z|0.75

# If X runs_on Y and Y is_a Z, then X runs_on Z (platform generalization)
platform_generalize|?x runs_on ?y|?y is_a ?z|?x runs_on ?z|0.8

# If X contains Y and Y is_a Z, then X has Z
containment_type|?x contains ?y|?y is_a ?z|?x has ?z|0.7

# If X comes_from Y and Y lives_in Z, then X related_to Z
origin_location|?x comes_from ?y|?y lives_in ?z|?x related_to ?z|0.6

# If X faster_than Y and Y faster_than Z, then X faster_than Z (transitive comparison)
speed_transitive|?x faster_than ?y|?y faster_than ?z|?x faster_than ?z|0.9

# If X larger_than Y and Y larger_than Z, then X larger_than Z
size_transitive|?x larger_than ?y|?y larger_than ?z|?x larger_than ?z|0.9

# If X created_by Y and Y knows Z, then X uses Z (creator knowledge transfers)
creator_knowledge|?x created_by ?y|?y knows ?z|?x built_with ?z|0.5

# If X is_a Y and Y used_for Z, then X used_for Z (purpose inheritance)
purpose_inherit|?x is_a ?y|?y used_for ?z|?x used_for ?z|0.8

# If X needs Y and Y is_a Z, then X needs Z (need generalization)
need_generalize|?x needs ?y|?y is_a ?z|?x needs ?z|0.65
