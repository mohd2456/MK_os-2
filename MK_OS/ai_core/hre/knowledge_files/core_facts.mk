# MK Core Knowledge Base
# Format: source|relation|target|weight
# Categories: animals, colors, geography, science, food, technology, general
# This is the foundation knowledge that MK starts with.

# === ANIMALS ===
cat|is_a|animal|1.0
cat|has|fur|1.0
cat|has|whiskers|1.0
cat|has|tail|1.0
cat|can|climb|1.0
cat|can|purr|1.0
cat|eats|fish|0.9
dog|is_a|animal|1.0
dog|has|fur|1.0
dog|has|tail|1.0
dog|can|bark|1.0
dog|can|fetch|0.9
dog|is_a|pet|0.9
bird|is_a|animal|1.0
bird|has|wings|1.0
bird|has|feathers|1.0
bird|can|fly|0.9
bird|can|sing|0.8
fish|is_a|animal|1.0
fish|lives_in|water|1.0
fish|has|fins|1.0
fish|can|swim|1.0
horse|is_a|animal|1.0
horse|can|run|1.0
horse|has|hooves|1.0
elephant|is_a|animal|1.0
elephant|has|trunk|1.0
elephant|is_a|mammal|1.0
animal|has|life|1.0
animal|needs|food|1.0
animal|needs|water|1.0
mammal|is_a|animal|1.0
mammal|has|warm blood|1.0

# === COLORS ===
sky|has|blue color|1.0
grass|has|green color|1.0
sun|has|yellow color|1.0
snow|has|white color|1.0
night|has|dark color|1.0
fire|has|red color|0.9
ocean|has|blue color|1.0

# === GEOGRAPHY ===
earth|is_a|planet|1.0
earth|has|oceans|1.0
earth|has|continents|1.0
sun|is_a|star|1.0
moon|orbits|earth|1.0
water|is_a|liquid|1.0
mountain|is_a|landform|1.0
ocean|is_a|body of water|1.0
river|is_a|body of water|1.0
desert|is_a|biome|1.0
forest|is_a|biome|1.0
africa|is_a|continent|1.0
asia|is_a|continent|1.0
europe|is_a|continent|1.0

# === BASIC SCIENCE ===
water|made_of|hydrogen and oxygen|1.0
ice|is_a|solid water|1.0
steam|is_a|water vapor|1.0
light|faster_than|sound|1.0
gravity|is_a|force|1.0
atom|is_a|particle|1.0
electricity|is_a|energy|1.0
oxygen|is_a|gas|1.0
carbon|is_a|element|1.0
plant|needs|sunlight|1.0
plant|produces|oxygen|1.0
tree|is_a|plant|1.0
flower|is_a|plant|1.0

# === FOOD ===
apple|is_a|fruit|1.0
banana|is_a|fruit|1.0
orange|is_a|fruit|1.0
pizza|is_a|food|1.0
bread|made_of|wheat|0.9
rice|is_a|grain|1.0
milk|comes_from|cow|1.0
egg|comes_from|chicken|1.0
chocolate|made_of|cocoa|1.0
fruit|is_a|food|1.0
vegetable|is_a|food|1.0

# === TECHNOLOGY ===
computer|is_a|machine|1.0
computer|has|processor|1.0
computer|has|memory|1.0
computer|uses|electricity|1.0
internet|is_a|network|1.0
phone|is_a|device|1.0
software|runs_on|computer|1.0
programming|is_a|skill|1.0
cpp|is_a|programming language|1.0
python|is_a|programming language|1.0
linux|is_a|operating system|1.0
macbook|is_a|laptop|1.0
laptop|is_a|computer|1.0

# === MK ITSELF ===
mk|is_a|ai system|1.0
mk|is_a|hybrid reasoning engine|1.0
mk|uses|pattern graphs|1.0
mk|uses|reasoning chains|1.0
mk|uses|template composition|1.0
mk|is_a|custom ai|1.0
mk|runs_on|low end hardware|1.0
mk|built_with|cpp|1.0
mk|has|instant learning|1.0
mk|has|knowledge graph|1.0
mk|is_a|operating system component|1.0

# === ADVANCED PHYSICS ===
speed of light|equals|299792458 meters per second|1.0
speed of light|symbol|c|1.0
planck constant|equals|6.626e-34 joule seconds|1.0
planck constant|symbol|h|1.0
quantum mechanics|describes|subatomic behavior|1.0
quantum mechanics|has|wave-particle duality|1.0
quantum mechanics|has|uncertainty principle|1.0
heisenberg uncertainty principle|states|cannot know position and momentum simultaneously|1.0
schrodinger equation|describes|quantum state evolution|1.0
superposition|is_a|quantum phenomenon|1.0
superposition|means|multiple states simultaneously|1.0
quantum entanglement|is_a|quantum phenomenon|1.0
quantum entanglement|connects|particles regardless of distance|1.0
photon|is_a|quantum of light|1.0
photon|has|zero rest mass|1.0
electron|is_a|subatomic particle|1.0
electron|has|negative charge|1.0
electron|mass|9.109e-31 kg|1.0
proton|is_a|subatomic particle|1.0
proton|has|positive charge|1.0
neutron|is_a|subatomic particle|1.0
neutron|has|no charge|1.0
quark|is_a|fundamental particle|1.0
quark|types|up down strange charm bottom top|1.0
quark|makes_up|protons and neutrons|1.0
gluon|is_a|force carrier|1.0
gluon|mediates|strong nuclear force|1.0
higgs boson|is_a|fundamental particle|1.0
higgs boson|gives|mass to particles|1.0
higgs boson|discovered|2012 at CERN|1.0
general relativity|describes|gravity as spacetime curvature|1.0
general relativity|proposed_by|albert einstein|1.0
general relativity|published|1915|1.0
special relativity|describes|motion at near light speed|1.0
special relativity|has|time dilation|1.0
special relativity|has|length contraction|1.0
e equals mc squared|relates|energy and mass|1.0
e equals mc squared|proposed_by|einstein|1.0
gravitational waves|predicted_by|general relativity|1.0
gravitational waves|detected|2015 by LIGO|1.0
black hole|is_a|region of spacetime|1.0
black hole|has|event horizon|1.0
black hole|has|singularity|1.0
thermodynamics|is_a|branch of physics|1.0
first law of thermodynamics|states|energy cannot be created or destroyed|1.0
second law of thermodynamics|states|entropy always increases|1.0
third law of thermodynamics|states|entropy approaches zero at absolute zero|1.0
entropy|is_a|measure of disorder|1.0
absolute zero|equals|0 kelvin or -273.15 celsius|1.0
electromagnetism|is_a|fundamental force|1.0
maxwell equations|describe|electromagnetism|1.0
electromagnetic spectrum|includes|radio microwave infrared visible ultraviolet xray gamma|1.0
nuclear fission|splits|heavy atomic nuclei|1.0
nuclear fusion|combines|light atomic nuclei|1.0
nuclear fusion|powers|the sun|1.0
strong force|is_a|fundamental force|1.0
strong force|holds|nucleus together|1.0
weak force|is_a|fundamental force|1.0
weak force|causes|radioactive decay|1.0
standard model|describes|fundamental particles and forces|1.0
standard model|has|17 fundamental particles|1.0
dark matter|makes_up|approximately 27 percent of universe|0.95
dark energy|makes_up|approximately 68 percent of universe|0.95
antimatter|is_a|mirror of matter|1.0
positron|is_a|antimatter electron|1.0

# === MATHEMATICS ===
pi|approximately_equals|3.14159265358979|1.0
pi|is_a|ratio of circumference to diameter|1.0
pi|is_a|irrational number|1.0
euler number e|approximately_equals|2.71828182845905|1.0
euler number e|is_a|base of natural logarithm|1.0
golden ratio phi|approximately_equals|1.61803398874989|1.0
golden ratio phi|appears_in|nature and art|1.0
pythagorean theorem|states|a squared plus b squared equals c squared|1.0
pythagorean theorem|applies_to|right triangles|1.0
fundamental theorem of calculus|connects|derivatives and integrals|1.0
fundamental theorem of algebra|states|every polynomial has a root in complex numbers|1.0
prime number|is_a|number divisible only by 1 and itself|1.0
prime number|examples|2 3 5 7 11 13 17 19 23|1.0
infinity|is_a|mathematical concept|1.0
infinity|is_not|a number|1.0
imaginary unit i|equals|square root of negative one|1.0
complex number|has|real and imaginary parts|1.0
fibonacci sequence|starts|0 1 1 2 3 5 8 13 21 34|1.0
fibonacci sequence|each_term|sum of previous two|1.0
euler formula|states|e to the i pi plus 1 equals 0|1.0
euler formula|connects|five fundamental constants|1.0
derivative|is_a|rate of change|1.0
integral|is_a|area under curve|1.0
limit|is_a|foundation of calculus|1.0
matrix|is_a|rectangular array of numbers|1.0
determinant|is_a|scalar value of matrix|1.0
eigenvalue|is_a|scalar in linear transformation|1.0
logarithm|is_a|inverse of exponentiation|1.0
natural logarithm|base|e|1.0
factorial|notation|n!|1.0
factorial|example|5! equals 120|1.0
set theory|is_a|branch of mathematics|1.0
boolean algebra|is_a|algebraic structure|1.0
boolean algebra|has|AND OR NOT operations|1.0
topology|studies|properties preserved under deformation|1.0
group theory|studies|algebraic structures with operations|1.0
bayes theorem|describes|conditional probability|1.0
normal distribution|is_a|bell curve|1.0
standard deviation|measures|spread of data|1.0
mean|is_a|average value|1.0
median|is_a|middle value|1.0
riemann hypothesis|is_a|unsolved problem|0.95
p versus np|is_a|unsolved problem|0.95
irrational number|cannot_be_expressed|as a simple fraction|1.0
transcendental number|is_a|non-algebraic number|1.0
transcendental number|examples|pi and e|1.0

# === WORLD GEOGRAPHY ===
north america|is_a|continent|1.0
south america|is_a|continent|1.0
australia|is_a|continent|1.0
antarctica|is_a|continent|1.0
pacific ocean|is_a|largest ocean|1.0
atlantic ocean|is_a|second largest ocean|1.0
indian ocean|is_a|third largest ocean|1.0
arctic ocean|is_a|smallest ocean|1.0
mount everest|is_a|tallest mountain|1.0
mount everest|height|8849 meters|1.0
mount everest|located_in|himalayas|1.0
mariana trench|is_a|deepest ocean point|1.0
mariana trench|depth|approximately 11034 meters|1.0
nile river|is_a|longest river|1.0
nile river|length|approximately 6650 km|1.0
amazon river|is_a|largest river by volume|1.0
amazon river|flows_through|south america|1.0
sahara|is_a|largest hot desert|1.0
sahara|located_in|north africa|1.0
united states|capital|washington dc|1.0
united states|continent|north america|1.0
united kingdom|capital|london|1.0
united kingdom|continent|europe|1.0
france|capital|paris|1.0
france|continent|europe|1.0
germany|capital|berlin|1.0
germany|continent|europe|1.0
japan|capital|tokyo|1.0
japan|continent|asia|1.0
china|capital|beijing|1.0
china|continent|asia|1.0
india|capital|new delhi|1.0
india|continent|asia|1.0
brazil|capital|brasilia|1.0
brazil|continent|south america|1.0
russia|capital|moscow|1.0
russia|spans|europe and asia|1.0
canada|capital|ottawa|1.0
canada|continent|north america|1.0
australia|capital|canberra|1.0
egypt|capital|cairo|1.0
egypt|continent|africa|1.0
mexico|capital|mexico city|1.0
mexico|continent|north america|1.0
south korea|capital|seoul|1.0
italy|capital|rome|1.0
spain|capital|madrid|1.0
argentina|capital|buenos aires|1.0
turkey|capital|ankara|1.0
himalayas|is_a|mountain range|1.0
himalayas|located_in|asia|1.0
alps|is_a|mountain range|1.0
alps|located_in|europe|1.0
andes|is_a|mountain range|1.0
andes|located_in|south america|1.0
rocky mountains|is_a|mountain range|1.0
rocky mountains|located_in|north america|1.0
great barrier reef|is_a|coral reef|1.0
great barrier reef|located_in|australia|1.0
mediterranean sea|connects|atlantic and asia minor|1.0

# === HISTORY ===
world war 1|started|1914|1.0
world war 1|ended|1918|1.0
world war 2|started|1939|1.0
world war 2|ended|1945|1.0
american revolution|year|1776|1.0
french revolution|year|1789|1.0
roman empire|fell|476 AD|1.0
roman empire|capital|rome|1.0
ancient egypt|built|pyramids|1.0
great pyramid of giza|built|approximately 2560 BC|1.0
printing press|invented_by|johannes gutenberg|1.0
printing press|invented|approximately 1440|1.0
telephone|invented_by|alexander graham bell|1.0
telephone|invented|1876|1.0
light bulb|invented_by|thomas edison|1.0
wright brothers|achieved|first powered flight|1.0
wright brothers|year|1903|1.0
moon landing|year|1969|1.0
moon landing|mission|apollo 11|1.0
neil armstrong|first_person_on|moon|1.0
industrial revolution|started|approximately 1760|1.0
industrial revolution|origin|great britain|1.0
renaissance|period|14th to 17th century|1.0
renaissance|origin|italy|1.0
cold war|period|1947 to 1991|1.0
berlin wall|fell|1989|1.0
declaration of independence|signed|1776|1.0
magna carta|signed|1215|1.0
isaac newton|discovered|laws of motion|1.0
isaac newton|published|principia mathematica 1687|1.0
charles darwin|proposed|theory of evolution|1.0
charles darwin|published|on the origin of species 1859|1.0
albert einstein|published|special relativity 1905|1.0
galileo galilei|proved|heliocentric model|1.0
copernicus|proposed|earth revolves around sun|1.0
alexander the great|conquered|persian empire|1.0
julius caesar|assassinated|44 BC|1.0
cleopatra|ruled|ancient egypt|1.0
genghis khan|founded|mongol empire|1.0
mongol empire|was|largest contiguous land empire|1.0

# === HUMAN BIOLOGY ===
human body|has|206 bones in adults|1.0
human body|has|approximately 37 trillion cells|0.95
heart|is_a|organ|1.0
heart|function|pumps blood|1.0
heart|beats|approximately 100000 times per day|0.95
brain|is_a|organ|1.0
brain|has|approximately 86 billion neurons|0.95
brain|controls|all body functions|1.0
dna|stands_for|deoxyribonucleic acid|1.0
dna|is_a|genetic code|1.0
dna|has|double helix structure|1.0
dna|discovered_by|watson and crick|1.0
rna|stands_for|ribonucleic acid|1.0
rna|used_for|protein synthesis|1.0
cell|is_a|basic unit of life|1.0
mitochondria|is_a|cell organelle|1.0
mitochondria|function|produces energy ATP|1.0
mitochondria|called|powerhouse of the cell|1.0
red blood cell|carries|oxygen|1.0
white blood cell|fights|infection|1.0
platelet|helps|blood clotting|1.0
lungs|function|gas exchange|1.0
lungs|exchange|oxygen and carbon dioxide|1.0
liver|is_a|organ|1.0
liver|function|detoxification and metabolism|1.0
kidneys|function|filter blood|1.0
immune system|protects|against disease|1.0
antibody|is_a|immune protein|1.0
antibody|fights|specific pathogens|1.0
vaccine|stimulates|immune response|1.0
vaccine|prevents|disease|1.0
chromosome|is_a|dna structure|1.0
human|has|23 pairs of chromosomes|1.0
gene|is_a|unit of heredity|1.0
mutation|is_a|change in dna sequence|1.0
evolution|is_a|change in species over time|1.0
natural selection|mechanism_of|evolution|1.0
natural selection|proposed_by|charles darwin|1.0
nervous system|has|central and peripheral divisions|1.0
neuron|transmits|electrical signals|1.0
synapse|is_a|junction between neurons|1.0
hormone|is_a|chemical messenger|1.0
insulin|regulates|blood sugar|1.0
adrenaline|triggers|fight or flight response|1.0
digestive system|breaks_down|food into nutrients|1.0
stomach|produces|hydrochloric acid|1.0
small intestine|absorbs|nutrients|1.0

# === ASTRONOMY ===
mercury|is_a|planet|1.0
mercury|position|closest to sun|1.0
venus|is_a|planet|1.0
venus|has|hottest surface temperature|1.0
earth|is_a|planet|1.0
earth|distance_from_sun|approximately 150 million km|1.0
mars|is_a|planet|1.0
mars|called|the red planet|1.0
jupiter|is_a|planet|1.0
jupiter|is_a|largest planet in solar system|1.0
jupiter|has|great red spot|1.0
saturn|is_a|planet|1.0
saturn|has|prominent ring system|1.0
uranus|is_a|planet|1.0
uranus|rotates|on its side|1.0
neptune|is_a|planet|1.0
neptune|is_a|farthest planet from sun|1.0
pluto|is_a|dwarf planet|1.0
pluto|reclassified|2006|1.0
sun|is_a|g-type main sequence star|1.0
sun|age|approximately 4.6 billion years|0.95
sun|composition|mostly hydrogen and helium|1.0
milky way|is_a|galaxy|1.0
milky way|type|barred spiral galaxy|1.0
milky way|contains|200 to 400 billion stars|0.95
andromeda|is_a|galaxy|1.0
andromeda|is_a|nearest large galaxy to milky way|1.0
light year|is_a|unit of distance|1.0
light year|equals|9.461 trillion kilometers|1.0
supernova|is_a|stellar explosion|1.0
supernova|creates|heavy elements|1.0
neutron star|is_a|collapsed stellar core|1.0
neutron star|extremely|dense|1.0
pulsar|is_a|rotating neutron star|1.0
pulsar|emits|beams of radiation|1.0
white dwarf|is_a|dead star remnant|1.0
red giant|is_a|late stage star|1.0
main sequence|is_a|stellar classification|1.0
big bang|is_a|origin of universe theory|0.95
big bang|occurred|approximately 13.8 billion years ago|0.95
cosmic microwave background|is_a|remnant of big bang|0.95
hubble constant|describes|rate of universe expansion|0.95
exoplanet|is_a|planet outside solar system|1.0
asteroid belt|located|between mars and jupiter|1.0
comet|is_a|icy body|1.0
comet|has|tail when near sun|1.0
international space station|orbits|earth|1.0
international space station|altitude|approximately 408 km|1.0
telescope|used_for|observing distant objects|1.0
james webb space telescope|launched|2021|1.0

# === CHEMISTRY ===
hydrogen|is_a|element|1.0
hydrogen|atomic_number|1|1.0
hydrogen|is_a|lightest element|1.0
helium|is_a|element|1.0
helium|atomic_number|2|1.0
helium|is_a|noble gas|1.0
oxygen|atomic_number|8|1.0
oxygen|essential_for|respiration|1.0
carbon|atomic_number|6|1.0
carbon|basis_of|organic chemistry|1.0
nitrogen|atomic_number|7|1.0
nitrogen|makes_up|78 percent of atmosphere|1.0
iron|atomic_number|26|1.0
iron|symbol|Fe|1.0
gold|atomic_number|79|1.0
gold|symbol|Au|1.0
periodic table|organized_by|atomic number|1.0
periodic table|has|118 elements|1.0
periodic table|created_by|dmitri mendeleev|1.0
noble gases|include|helium neon argon krypton xenon radon|1.0
noble gases|are|chemically inert|1.0
alkali metals|include|lithium sodium potassium|1.0
alkali metals|are|highly reactive|1.0
covalent bond|shares|electrons between atoms|1.0
ionic bond|transfers|electrons between atoms|1.0
metallic bond|has|sea of electrons|1.0
acid|has|ph below 7|1.0
base|has|ph above 7|1.0
neutral|has|ph of 7|1.0
ph scale|ranges|0 to 14|1.0
oxidation|is_a|loss of electrons|1.0
reduction|is_a|gain of electrons|1.0
catalyst|speeds_up|chemical reaction|1.0
catalyst|not_consumed|in reaction|1.0
exothermic reaction|releases|heat|1.0
endothermic reaction|absorbs|heat|1.0
mole|equals|6.022e23 particles|1.0
mole|is_a|avogadro number|1.0
water molecule|formula|H2O|1.0
carbon dioxide|formula|CO2|1.0
sodium chloride|formula|NaCl|1.0
sodium chloride|common_name|table salt|1.0
organic chemistry|studies|carbon compounds|1.0
polymer|is_a|chain of repeating units|1.0
isotope|has|same protons different neutrons|1.0
radioactive decay|is_a|nuclear process|1.0
half life|is_a|time for half decay|1.0

# === PHILOSOPHY ===
socrates|is_a|greek philosopher|1.0
socrates|method|socratic questioning|1.0
socrates|student|plato|1.0
plato|is_a|greek philosopher|1.0
plato|wrote|the republic|1.0
plato|student|aristotle|1.0
aristotle|is_a|greek philosopher|1.0
aristotle|founded|formal logic|1.0
aristotle|teacher_of|alexander the great|1.0
descartes|is_a|philosopher|1.0
descartes|said|i think therefore i am|1.0
descartes|founder_of|modern rationalism|1.0
kant|is_a|philosopher|1.0
kant|wrote|critique of pure reason|1.0
kant|proposed|categorical imperative|1.0
nietzsche|is_a|philosopher|1.0
nietzsche|concept|will to power|1.0
nietzsche|concept|eternal recurrence|1.0
existentialism|is_a|philosophical school|1.0
existentialism|emphasizes|individual freedom and choice|1.0
existentialism|thinkers|sartre kierkegaard camus|1.0
stoicism|is_a|philosophical school|1.0
stoicism|emphasizes|virtue and self-control|1.0
stoicism|thinkers|marcus aurelius epictetus seneca|1.0
utilitarianism|is_a|ethical theory|1.0
utilitarianism|seeks|greatest happiness for greatest number|1.0
utilitarianism|founders|bentham and mill|1.0
empiricism|is_a|philosophical school|1.0
empiricism|holds|knowledge comes from experience|1.0
rationalism|is_a|philosophical school|1.0
rationalism|holds|knowledge comes from reason|1.0
determinism|is_a|philosophical position|1.0
determinism|holds|all events are caused|1.0
free will|is_a|philosophical concept|1.0
dualism|is_a|philosophical position|1.0
dualism|separates|mind and body|1.0
materialism|is_a|philosophical position|1.0
materialism|holds|only matter exists|1.0
ethics|is_a|branch of philosophy|1.0
epistemology|is_a|branch of philosophy|1.0
epistemology|studies|nature of knowledge|1.0
metaphysics|is_a|branch of philosophy|1.0
metaphysics|studies|nature of reality|1.0
logic|is_a|branch of philosophy|1.0
logic|studies|valid reasoning|1.0
aesthetics|is_a|branch of philosophy|1.0
aesthetics|studies|beauty and art|1.0
confucius|is_a|chinese philosopher|1.0
confucius|emphasized|social harmony and virtue|1.0
buddha|founded|buddhism|1.0
buddha|taught|four noble truths|1.0
