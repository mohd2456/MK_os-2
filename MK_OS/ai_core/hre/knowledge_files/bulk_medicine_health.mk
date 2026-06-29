# Medicine & Health Knowledge
# Format: source|relation|target|weight
# Covers: human body systems, diseases, vitamins, first aid, mental health

# Cardiovascular System
heart|is_a|organ|1.0
heart|function_is|pumping blood|1.0
heart|has_part|left ventricle|0.9
heart|has_part|right ventricle|0.9
heart|has_part|left atrium|0.9
heart|has_part|right atrium|0.9
heart|beats_per_minute|60 to 100 at rest|0.95
heart|weighs_approximately|300 grams|0.9
blood|contains|red blood cells|1.0
blood|contains|white blood cells|1.0
blood|contains|platelets|1.0
blood|contains|plasma|1.0
red_blood_cells|function_is|carrying oxygen|1.0
white_blood_cells|function_is|fighting infection|1.0
platelets|function_is|blood clotting|1.0
arteries|carry|oxygenated blood|0.95
veins|carry|deoxygenated blood|0.95
capillaries|are|smallest blood vessels|0.95
aorta|is_a|largest artery|1.0
blood_pressure|normal_range|120/80 mmHg|0.95
hypertension|is_a|high blood pressure|1.0
hypertension|risk_factor_for|heart attack|0.9
hypertension|risk_factor_for|stroke|0.9
atherosclerosis|is_a|artery plaque buildup|0.95
cholesterol|type|HDL good cholesterol|0.9
cholesterol|type|LDL bad cholesterol|0.9

# Respiratory System
lungs|is_a|organ|1.0
lungs|function_is|gas exchange|1.0
lungs|has_part|alveoli|0.95
lungs|has_part|bronchi|0.95
lungs|has_part|bronchioles|0.9
diaphragm|function_is|breathing muscle|1.0
trachea|is_a|windpipe|1.0
oxygen|enters_blood_via|alveoli|0.95
carbon_dioxide|exits_blood_via|alveoli|0.95
respiratory_rate|normal_adult|12 to 20 breaths per minute|0.95
asthma|is_a|chronic respiratory disease|1.0
asthma|causes|airway inflammation|0.95
pneumonia|is_a|lung infection|1.0
copd|is_a|chronic obstructive pulmonary disease|1.0
bronchitis|is_a|bronchial inflammation|0.95

# Nervous System
brain|is_a|organ|1.0
brain|weighs_approximately|1.4 kilograms|0.95
brain|has_part|cerebrum|1.0
brain|has_part|cerebellum|1.0
brain|has_part|brainstem|1.0
brain|contains_approximately|86 billion neurons|0.9
cerebrum|function_is|thinking and voluntary movement|0.95
cerebellum|function_is|balance and coordination|0.95
brainstem|function_is|basic life functions|0.95
neuron|is_a|nerve cell|1.0
neuron|communicates_via|synapses|0.95
neurotransmitter|example|dopamine|0.9
neurotransmitter|example|serotonin|0.9
neurotransmitter|example|acetylcholine|0.9
dopamine|associated_with|reward and motivation|0.9
serotonin|associated_with|mood regulation|0.9
spinal_cord|function_is|nerve signal relay|0.95
peripheral_nervous_system|includes|sensory nerves|0.9
peripheral_nervous_system|includes|motor nerves|0.9
autonomic_nervous_system|controls|involuntary functions|0.95
alzheimers|is_a|neurodegenerative disease|1.0
parkinsons|is_a|neurodegenerative disease|1.0
epilepsy|is_a|neurological disorder|1.0
multiple_sclerosis|is_a|autoimmune neurological disease|1.0

# Digestive System
stomach|is_a|organ|1.0
stomach|function_is|food digestion|1.0
stomach|produces|hydrochloric acid|0.95
liver|is_a|organ|1.0
liver|function_is|detoxification|0.95
liver|function_is|bile production|0.95
liver|is_the|largest internal organ|0.95
pancreas|produces|insulin|0.95
pancreas|produces|digestive enzymes|0.95
small_intestine|function_is|nutrient absorption|1.0
large_intestine|function_is|water absorption|0.95
esophagus|connects|mouth to stomach|0.95
gallbladder|stores|bile|0.95
appendix|is_a|vestigial organ|0.8
gut_microbiome|contains|trillions of bacteria|0.95
celiac_disease|triggered_by|gluten|0.95
crohns_disease|is_a|inflammatory bowel disease|1.0

# Immune System
immune_system|function_is|fighting pathogens|1.0
antibodies|produced_by|B lymphocytes|0.95
t_cells|function_is|killing infected cells|0.95
vaccination|works_by|training immune memory|0.95
inflammation|is_a|immune response|0.9
fever|is_a|immune defense mechanism|0.9
lymph_nodes|function_is|filtering lymph fluid|0.95
bone_marrow|produces|blood cells|0.95
thymus|trains|T cells|0.9
spleen|filters|old blood cells|0.9
autoimmune_disease|happens_when|immune system attacks own tissue|0.95
allergy|is_a|overactive immune response|0.9
histamine|causes|allergic symptoms|0.9
anaphylaxis|is_a|severe allergic reaction|1.0

# Common Diseases
diabetes_type1|is_a|autoimmune disease|1.0
diabetes_type1|destroys|insulin producing cells|0.95
diabetes_type2|is_a|metabolic disorder|1.0
diabetes_type2|characterized_by|insulin resistance|0.95
cancer|is_a|uncontrolled cell growth|1.0
cancer|types_include|carcinoma|0.9
cancer|types_include|sarcoma|0.9
cancer|types_include|leukemia|0.9
cancer|types_include|lymphoma|0.9
influenza|is_a|viral infection|1.0
influenza|spread_by|respiratory droplets|0.95
common_cold|caused_by|rhinovirus usually|0.9
covid19|caused_by|SARS-CoV-2|1.0
malaria|caused_by|plasmodium parasite|1.0
malaria|transmitted_by|mosquitoes|1.0
tuberculosis|caused_by|mycobacterium|1.0
hiv|attacks|CD4 T cells|0.95
aids|caused_by|HIV progression|1.0
stroke|types|ischemic and hemorrhagic|0.95
heart_attack|caused_by|coronary artery blockage|0.95

# Vitamins and Minerals
vitamin_a|function_is|vision and immunity|0.95
vitamin_a|found_in|carrots and liver|0.9
vitamin_b12|function_is|nerve function and blood formation|0.95
vitamin_b12|found_in|meat and dairy|0.9
vitamin_c|function_is|collagen synthesis and antioxidant|0.95
vitamin_c|found_in|citrus fruits|0.9
vitamin_c|deficiency_causes|scurvy|0.95
vitamin_d|function_is|calcium absorption|0.95
vitamin_d|source|sunlight|0.95
vitamin_d|deficiency_causes|rickets|0.95
vitamin_e|function_is|antioxidant protection|0.9
vitamin_k|function_is|blood clotting|0.95
iron|function_is|oxygen transport in hemoglobin|0.95
iron|deficiency_causes|anemia|0.95
calcium|function_is|bone and teeth strength|0.95
potassium|function_is|muscle and nerve function|0.9
zinc|function_is|immune function and wound healing|0.9
magnesium|function_is|muscle relaxation and energy|0.9
iodine|function_is|thyroid hormone production|0.95
folate|function_is|DNA synthesis and cell division|0.95
folate|important_during|pregnancy|0.95

# First Aid
cpr|stands_for|cardiopulmonary resuscitation|1.0
cpr|compression_rate|100 to 120 per minute|0.95
cpr|compression_depth|2 inches for adults|0.95
heimlich_maneuver|used_for|choking|1.0
recovery_position|used_for|unconscious breathing person|0.95
tourniquet|used_for|severe limb bleeding|0.9
burns|first_aid|cool with running water 20 minutes|0.95
fracture|first_aid|immobilize and seek help|0.95
nosebleed|first_aid|lean forward pinch nose|0.95
hypothermia|signs|shivering confusion drowsiness|0.9
heatstroke|signs|high temp confusion no sweating|0.9
aed|stands_for|automated external defibrillator|1.0
aed|used_for|cardiac arrest|0.95
epipen|contains|epinephrine|1.0
epipen|used_for|anaphylaxis|1.0
shock|signs|pale cold skin rapid pulse|0.9
wound|cleaning|rinse with clean water|0.9

# Mental Health
depression|is_a|mood disorder|1.0
depression|symptoms|persistent sadness low energy|0.95
depression|treatment|therapy and medication|0.9
anxiety|is_a|mental health condition|1.0
anxiety|types|generalized social panic|0.9
ptsd|stands_for|post traumatic stress disorder|1.0
ptsd|triggered_by|traumatic events|0.95
bipolar_disorder|characterized_by|mood swings mania and depression|0.95
schizophrenia|is_a|psychotic disorder|1.0
schizophrenia|symptoms|hallucinations delusions|0.95
ocd|stands_for|obsessive compulsive disorder|1.0
adhd|stands_for|attention deficit hyperactivity disorder|1.0
adhd|characterized_by|inattention hyperactivity impulsivity|0.95
therapy|type|cognitive behavioral therapy|0.95
therapy|type|dialectical behavior therapy|0.9
therapy|type|psychodynamic therapy|0.85
meditation|benefits|stress reduction improved focus|0.9
exercise|benefits_mental_health|releases endorphins|0.9
sleep|recommended_hours|7 to 9 for adults|0.95
insomnia|is_a|sleep disorder|0.95
eating_disorder|types|anorexia bulimia binge eating|0.95

# Musculoskeletal System
skeleton|contains_approximately|206 bones in adults|0.95
femur|is_a|longest bone in body|1.0
skull|protects|brain|1.0
ribcage|protects|heart and lungs|0.95
spine|has|33 vertebrae|0.95
joints|types|ball and socket hinge pivot|0.9
cartilage|function_is|cushioning joints|0.95
tendons|connect|muscle to bone|1.0
ligaments|connect|bone to bone|1.0
osteoporosis|is_a|bone density loss|0.95
arthritis|is_a|joint inflammation|1.0
arthritis|types|osteoarthritis rheumatoid|0.9
muscle|types|skeletal cardiac smooth|0.95
skeletal_muscle|is|voluntary|0.95
cardiac_muscle|is|involuntary|0.95

# Endocrine System
thyroid|produces|T3 and T4 hormones|0.95
thyroid|regulates|metabolism|0.95
adrenal_glands|produce|cortisol and adrenaline|0.95
pituitary_gland|is_a|master gland|0.9
pituitary_gland|controls|other endocrine glands|0.9
insulin|lowers|blood sugar|1.0
glucagon|raises|blood sugar|0.95
estrogen|is_a|female sex hormone|0.95
testosterone|is_a|male sex hormone|0.95
melatonin|regulates|sleep wake cycle|0.95
cortisol|is_a|stress hormone|0.9
growth_hormone|produced_by|pituitary gland|0.95

# Pharmacology Basics
antibiotic|kills|bacteria|0.95
antibiotic|does_not_work_on|viruses|1.0
antibiotic_resistance|caused_by|overuse of antibiotics|0.95
penicillin|discovered_by|alexander fleming|1.0
penicillin|discovered_in|1928|1.0
aspirin|is_a|pain reliever and blood thinner|0.9
ibuprofen|is_a|NSAID anti inflammatory|0.95
acetaminophen|is_a|pain and fever reducer|0.95
vaccine|contains|weakened or inactivated pathogen|0.9
mrna_vaccine|works_by|teaching cells to make spike protein|0.9
placebo_effect|is_a|improvement from belief in treatment|0.9
drug_half_life|is|time to reduce drug concentration by half|0.95
side_effect|is_a|unintended drug effect|0.9

# Additional Medical Knowledge
dna|stands_for|deoxyribonucleic acid|1.0
dna|structure|double helix|1.0
dna|discovered_structure_by|watson and crick|0.95
rna|stands_for|ribonucleic acid|1.0
rna|types|mRNA tRNA rRNA|0.9
gene|is_a|unit of heredity|1.0
chromosome|humans_have|46 in 23 pairs|0.95
mitosis|is|cell division for growth|0.95
meiosis|is|cell division for gametes|0.95
stem_cells|can_become|any cell type|0.95
red_blood_cell_lifespan|is|about 120 days|0.9
platelet_lifespan|is|about 8 to 10 days|0.85
white_blood_cell_types|include|neutrophils lymphocytes monocytes|0.9
hemoglobin|carries|oxygen in red blood cells|1.0
blood_types|include|A B AB O|1.0
rh_factor|types|positive and negative|0.95
universal_donor|blood_type|O negative|0.95
universal_recipient|blood_type|AB positive|0.95
kidney|function_is|filtering blood and making urine|0.95
kidney|each_contains|about 1 million nephrons|0.9
dialysis|replaces|kidney function|0.95
liver_regeneration|can_regrow|from 25 percent|0.9
skin|is_the|largest organ by area|1.0
skin|layers|epidermis dermis hypodermis|0.95
melanin|determines|skin color|0.95
collagen|is_the|most abundant protein in body|0.9
keratin|makes_up|hair and nails|0.9
tendon_achilles|is_the|strongest tendon|0.9
funny_bone|is_actually|ulnar nerve|0.9
cornea|has_no|blood vessels|0.9
retina|contains|rods and cones for vision|0.95
cochlea|converts|sound to nerve signals|0.95
vestibular_system|controls|balance|0.95
olfactory_nerve|detects|smell|0.9
taste_buds|detect|sweet sour salty bitter umami|0.95
body_temperature|normal|37 degrees celsius|0.95
blood_ph|normal_range|7.35 to 7.45|0.95
respiratory_alkalosis|caused_by|hyperventilation|0.85
metabolic_acidosis|caused_by|kidney failure or diabetes|0.85
bmi|stands_for|body mass index|0.95
calorie|is_a|unit of energy|0.95
basal_metabolic_rate|is|energy used at rest|0.9
protein|made_of|amino acids|1.0
essential_amino_acids|cannot_be|made by the body|0.95
carbohydrates|broken_into|glucose for energy|0.95
saturated_fat|raises|LDL cholesterol|0.9
unsaturated_fat|is|healthier fat type|0.9
omega_3|found_in|fish and flaxseed|0.9
omega_3|benefits|heart and brain health|0.85
fiber|aids|digestion and gut health|0.9
probiotics|are|beneficial gut bacteria|0.9
antioxidant|protects|cells from free radical damage|0.9
free_radicals|cause|oxidative stress|0.9
cortisol|elevated_by|chronic stress|0.9
chronic_stress|increases_risk_of|heart disease and depression|0.9
circadian_rhythm|is|24 hour biological clock|0.95
melatonin|produced_by|pineal gland at night|0.95
rem_sleep|is_when|most dreaming occurs|0.95
sleep_stages|include|NREM 1 2 3 and REM|0.9
sleep_debt|accumulates_from|insufficient sleep|0.85
caffeine|blocks|adenosine receptors|0.9
caffeine|half_life|about 5 hours|0.85
alcohol|is_a|CNS depressant|0.95
nicotine|is_a|stimulant and addictive|0.95
dopamine_pathway|involved_in|addiction|0.9
serotonin_syndrome|caused_by|excess serotonin|0.85
blood_brain_barrier|protects|brain from toxins|0.95
general_anesthesia|causes|reversible unconsciousness|0.9
local_anesthesia|blocks|nerve signals in area|0.95
mri|stands_for|magnetic resonance imaging|1.0
ct_scan|uses|x rays for cross sections|0.95
ultrasound|uses|sound waves for imaging|0.95
ecg|stands_for|electrocardiogram|1.0
ecg|measures|heart electrical activity|0.95
stethoscope|invented_by|rene laennec|0.9
blood_pressure_cuff|invented_by|samuel von basch|0.85
hippocratic_oath|is|ethical code for doctors|0.95
hippocrates|known_as|father of medicine|0.95
galen|is_a|ancient roman physician|0.9
pasteur|known_for|germ theory and pasteurization|0.95
lister|known_for|antiseptic surgery|0.9
jenner|known_for|smallpox vaccine|0.95
fleming|known_for|discovering penicillin|1.0
crispr|is_a|gene editing technology|0.95
crispr|discovered_by|doudna and charpentier|0.9
organ_transplant|first_successful|kidney in 1954|0.9
heart_transplant|first_by|christiaan barnard 1967|0.9

# Nutrition and Wellness
vitamin_b1|also_called|thiamine|0.9
vitamin_b2|also_called|riboflavin|0.9
vitamin_b3|also_called|niacin|0.9
vitamin_b6|function_is|brain development and function|0.9
biotin|also_called|vitamin B7|0.85
pantothenic_acid|also_called|vitamin B5|0.85
sodium|excess_causes|high blood pressure|0.9
phosphorus|function_is|bone formation and energy|0.85
selenium|function_is|antioxidant and thyroid function|0.85
chromium|function_is|insulin function|0.8
manganese|function_is|bone health and metabolism|0.8
copper|function_is|iron metabolism and immunity|0.85
fluoride|function_is|tooth enamel strengthening|0.9
dehydration|symptoms|thirst dizziness dry mouth|0.9
electrolytes|include|sodium potassium chloride|0.95
glycemic_index|measures|how fast food raises blood sugar|0.9
intermittent_fasting|is|cycling between eating and fasting|0.85
ketosis|is|burning fat for energy instead of carbs|0.85
metabolism|affected_by|age muscle mass activity level|0.9
bmi_categories|include|underweight normal overweight obese|0.9
aerobic_exercise|strengthens|heart and lungs|0.95
anaerobic_exercise|builds|muscle and strength|0.9
stretching|improves|flexibility and prevents injury|0.9
resting_heart_rate|athlete|40 to 60 bpm|0.85
vo2_max|measures|maximum oxygen uptake|0.9
lactic_acid|produced_during|intense exercise|0.9
endorphins|released_during|exercise|0.95
cortisol|released_during|stress|0.95
adrenaline|triggers|fight or flight response|0.95
parasympathetic_nervous_system|controls|rest and digest|0.9
sympathetic_nervous_system|controls|fight or flight|0.9
vagus_nerve|longest|cranial nerve|0.9
vagus_nerve|regulates|heart rate and digestion|0.9

# Infectious Diseases Expanded
mrsa|stands_for|methicillin resistant staph aureus|0.9
ebola|is_a|viral hemorrhagic fever|0.95
zika|transmitted_by|aedes mosquitoes|0.9
dengue|transmitted_by|aedes mosquitoes|0.9
cholera|caused_by|vibrio cholerae|0.95
typhoid|caused_by|salmonella typhi|0.95
tetanus|caused_by|clostridium tetani|0.95
rabies|transmitted_by|animal bites|0.95
measles|is_a|highly contagious viral disease|0.95
mumps|affects|salivary glands|0.9
rubella|also_called|german measles|0.9
whooping_cough|also_called|pertussis|0.9
chickenpox|caused_by|varicella zoster virus|0.95
shingles|reactivation_of|varicella zoster virus|0.95
hpv|stands_for|human papillomavirus|0.95
hepatitis|types|A B C D E|0.95
hepatitis_b|transmitted_by|blood and body fluids|0.95
prion_disease|caused_by|misfolded proteins|0.9
anthrax|caused_by|bacillus anthracis|0.9
leprosy|caused_by|mycobacterium leprae|0.9
polio|nearly_eradicated_by|vaccination|0.95
smallpox|officially_eradicated|1980|1.0
hand_hygiene|reduces|infection transmission by 50 percent|0.9
antibiotic_resistance|is_a|global health threat|0.95
superbugs|are|bacteria resistant to multiple antibiotics|0.9

# Reproductive and Developmental Health
pregnancy|duration|approximately 40 weeks|0.95
trimester|is|13 week period of pregnancy|0.9
embryo|becomes_fetus_at|week 9|0.9
fetal_heartbeat|detectable_at|about 6 weeks|0.9
placenta|provides|nutrients and oxygen to fetus|0.95
amniotic_fluid|protects|developing fetus|0.9
ultrasound|used_for|prenatal imaging|0.95
gestational_diabetes|occurs_during|pregnancy|0.9
preeclampsia|is|high blood pressure in pregnancy|0.9
cesarean_section|is|surgical delivery|0.95
breastfeeding|provides|antibodies to infant|0.95
colostrum|is|first breast milk rich in antibodies|0.9
apgar_score|assesses|newborn health at birth|0.9
neonatal|refers_to|first 28 days of life|0.9
puberty|age_range|8 to 14 typically|0.9
menopause|average_age|51 years|0.9
testosterone_peaks|in|late teens to early twenties|0.85
fertility|declines_after|age 35 in women|0.85
ivf|stands_for|in vitro fertilization|1.0
contraception|methods|hormonal barrier surgical|0.9

# Surgery and Procedures
appendectomy|removes|appendix|0.95
cholecystectomy|removes|gallbladder|0.9
tonsillectomy|removes|tonsils|0.95
knee_replacement|is|total joint arthroplasty|0.9
hip_replacement|lifespan|15 to 25 years|0.85
cataract_surgery|replaces|clouded lens with artificial|0.95
lasik|reshapes|cornea for vision correction|0.95
colonoscopy|screens_for|colon cancer|0.95
endoscopy|examines|digestive tract interior|0.9
biopsy|removes|tissue sample for analysis|0.95
chemotherapy|kills|rapidly dividing cells|0.95
radiation_therapy|uses|high energy beams to kill cancer|0.95
immunotherapy|boosts|immune system against cancer|0.9
targeted_therapy|attacks|specific cancer molecules|0.9
bone_marrow_transplant|replaces|damaged bone marrow|0.9
dialysis_types|include|hemodialysis and peritoneal|0.9
stent|keeps|blood vessel open|0.9
pacemaker|regulates|heart rhythm|0.95
defibrillator|restores|normal heart rhythm with shock|0.95
ventilator|assists|breathing mechanically|0.95
angioplasty|opens|blocked arteries|0.9
bypass_surgery|creates|new path for blood flow|0.9
laparoscopy|is|minimally invasive surgery|0.9
robotic_surgery|uses|robotic arms for precision|0.85

# Public Health
epidemiology|studies|disease patterns in populations|0.95
pandemic|is|worldwide disease outbreak|1.0
epidemic|is|regional disease outbreak|0.95
endemic|is|disease constantly present in area|0.9
quarantine|isolates|exposed individuals|0.95
contact_tracing|identifies|exposed persons|0.9
herd_immunity|threshold|varies 70 to 95 percent|0.9
r_naught|measures|disease transmission rate|0.95
incubation_period|is|time from exposure to symptoms|0.95
mortality_rate|is|deaths per infected|0.95
morbidity_rate|is|illness cases per population|0.9
who|stands_for|world health organization|1.0
cdc|stands_for|centers for disease control|1.0
clinical_trial|phases|1 2 3 4|0.95
placebo_controlled|compares|treatment to inactive substance|0.95
double_blind|means|neither patient nor doctor knows treatment|0.95
informed_consent|required_before|medical procedures|0.95
triage|prioritizes|patients by severity|0.95
emergency_medicine|treats|acute conditions|0.9
primary_care|provides|first point of healthcare contact|0.9

# Genetics and Modern Medicine
human_genome|contains|about 3 billion base pairs|0.95
human_genome_project|completed_in|2003|0.95
gene_therapy|replaces|faulty genes with correct ones|0.9
pharmacogenomics|tailors|drugs to genetics|0.85
personalized_medicine|uses|individual genetic info|0.9
biomarker|indicates|disease state or response|0.9
monoclonal_antibody|targets|specific proteins|0.9
car_t_therapy|engineers|immune cells to fight cancer|0.85
mrna_technology|used_in|covid vaccines|0.95
pcr|stands_for|polymerase chain reaction|1.0
pcr|amplifies|DNA for testing|0.95
next_gen_sequencing|reads|DNA rapidly|0.9
genetic_counseling|assesses|hereditary disease risk|0.85
autosomal_dominant|needs|one copy of mutant gene|0.9
autosomal_recessive|needs|two copies of mutant gene|0.9
x_linked|carried_on|X chromosome|0.9
down_syndrome|caused_by|trisomy 21|0.95
cystic_fibrosis|is_a|autosomal recessive disease|0.95
sickle_cell_disease|is_a|hemoglobin disorder|0.95
huntingtons_disease|is_a|autosomal dominant disorder|0.9
phenylketonuria|is_a|metabolic genetic disorder|0.85
epigenetics_mechanisms|include|methylation and histone modification|0.85
telomere|shortens|with each cell division|0.9
telomerase|enzyme|that extends telomeres|0.9
apoptosis|is|programmed cell death|0.95
autophagy|is|cell self cleaning process|0.9
microbiome|contains|trillions of microorganisms|0.95
fecal_transplant|treats|C difficile infection|0.85
antiretroviral_therapy|controls|HIV replication|0.95
prep|stands_for|pre exposure prophylaxis|0.9
prep|prevents|HIV infection|0.9
