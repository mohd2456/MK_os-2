"""Science facts: all 118 elements, physics, chemistry, biology."""

def get_elements():
    """All 118 elements with atomic number, symbol, category."""
    elements = [
        (1,"hydrogen","H","nonmetal","1.008"),(2,"helium","He","noble gas","4.003"),
        (3,"lithium","Li","alkali metal","6.941"),(4,"beryllium","Be","alkaline earth metal","9.012"),
        (5,"boron","B","metalloid","10.81"),(6,"carbon","C","nonmetal","12.011"),
        (7,"nitrogen","N","nonmetal","14.007"),(8,"oxygen","O","nonmetal","15.999"),
        (9,"fluorine","F","halogen","18.998"),(10,"neon","Ne","noble gas","20.180"),
        (11,"sodium","Na","alkali metal","22.990"),(12,"magnesium","Mg","alkaline earth metal","24.305"),
        (13,"aluminum","Al","post-transition metal","26.982"),(14,"silicon","Si","metalloid","28.086"),
        (15,"phosphorus","P","nonmetal","30.974"),(16,"sulfur","S","nonmetal","32.065"),
        (17,"chlorine","Cl","halogen","35.453"),(18,"argon","Ar","noble gas","39.948"),
        (19,"potassium","K","alkali metal","39.098"),(20,"calcium","Ca","alkaline earth metal","40.078"),
        (21,"scandium","Sc","transition metal","44.956"),(22,"titanium","Ti","transition metal","47.867"),
        (23,"vanadium","V","transition metal","50.942"),(24,"chromium","Cr","transition metal","51.996"),
        (25,"manganese","Mn","transition metal","54.938"),(26,"iron","Fe","transition metal","55.845"),
        (27,"cobalt","Co","transition metal","58.933"),(28,"nickel","Ni","transition metal","58.693"),
        (29,"copper","Cu","transition metal","63.546"),(30,"zinc","Zn","transition metal","65.38"),
        (31,"gallium","Ga","post-transition metal","69.723"),(32,"germanium","Ge","metalloid","72.630"),
        (33,"arsenic","As","metalloid","74.922"),(34,"selenium","Se","nonmetal","78.971"),
        (35,"bromine","Br","halogen","79.904"),(36,"krypton","Kr","noble gas","83.798"),
        (37,"rubidium","Rb","alkali metal","85.468"),(38,"strontium","Sr","alkaline earth metal","87.62"),
        (39,"yttrium","Y","transition metal","88.906"),(40,"zirconium","Zr","transition metal","91.224"),
        (41,"niobium","Nb","transition metal","92.906"),(42,"molybdenum","Mo","transition metal","95.95"),
        (43,"technetium","Tc","transition metal","98"),(44,"ruthenium","Ru","transition metal","101.07"),
        (45,"rhodium","Rh","transition metal","102.91"),(46,"palladium","Pd","transition metal","106.42"),
        (47,"silver","Ag","transition metal","107.87"),(48,"cadmium","Cd","transition metal","112.41"),
        (49,"indium","In","post-transition metal","114.82"),(50,"tin","Sn","post-transition metal","118.71"),
        (51,"antimony","Sb","metalloid","121.76"),(52,"tellurium","Te","metalloid","127.60"),
        (53,"iodine","I","halogen","126.90"),(54,"xenon","Xe","noble gas","131.29"),
        (55,"cesium","Cs","alkali metal","132.91"),(56,"barium","Ba","alkaline earth metal","137.33"),
        (57,"lanthanum","La","lanthanide","138.91"),(58,"cerium","Ce","lanthanide","140.12"),
        (59,"praseodymium","Pr","lanthanide","140.91"),(60,"neodymium","Nd","lanthanide","144.24"),
        (61,"promethium","Pm","lanthanide","145"),(62,"samarium","Sm","lanthanide","150.36"),
        (63,"europium","Eu","lanthanide","151.96"),(64,"gadolinium","Gd","lanthanide","157.25"),
        (65,"terbium","Tb","lanthanide","158.93"),(66,"dysprosium","Dy","lanthanide","162.50"),
        (67,"holmium","Ho","lanthanide","164.93"),(68,"erbium","Er","lanthanide","167.26"),
        (69,"thulium","Tm","lanthanide","168.93"),(70,"ytterbium","Yb","lanthanide","173.05"),
        (71,"lutetium","Lu","lanthanide","174.97"),(72,"hafnium","Hf","transition metal","178.49"),
        (73,"tantalum","Ta","transition metal","180.95"),(74,"tungsten","W","transition metal","183.84"),
        (75,"rhenium","Re","transition metal","186.21"),(76,"osmium","Os","transition metal","190.23"),
        (77,"iridium","Ir","transition metal","192.22"),(78,"platinum","Pt","transition metal","195.08"),
        (79,"gold","Au","transition metal","196.97"),(80,"mercury","Hg","transition metal","200.59"),
        (81,"thallium","Tl","post-transition metal","204.38"),(82,"lead","Pb","post-transition metal","207.2"),
        (83,"bismuth","Bi","post-transition metal","208.98"),(84,"polonium","Po","post-transition metal","209"),
        (85,"astatine","At","halogen","210"),(86,"radon","Rn","noble gas","222"),
        (87,"francium","Fr","alkali metal","223"),(88,"radium","Ra","alkaline earth metal","226"),
        (89,"actinium","Ac","actinide","227"),(90,"thorium","Th","actinide","232.04"),
        (91,"protactinium","Pa","actinide","231.04"),(92,"uranium","U","actinide","238.03"),
        (93,"neptunium","Np","actinide","237"),(94,"plutonium","Pu","actinide","244"),
        (95,"americium","Am","actinide","243"),(96,"curium","Cm","actinide","247"),
        (97,"berkelium","Bk","actinide","247"),(98,"californium","Cf","actinide","251"),
        (99,"einsteinium","Es","actinide","252"),(100,"fermium","Fm","actinide","257"),
        (101,"mendelevium","Md","actinide","258"),(102,"nobelium","No","actinide","259"),
        (103,"lawrencium","Lr","actinide","266"),(104,"rutherfordium","Rf","transition metal","267"),
        (105,"dubnium","Db","transition metal","268"),(106,"seaborgium","Sg","transition metal","269"),
        (107,"bohrium","Bh","transition metal","270"),(108,"hassium","Hs","transition metal","277"),
        (109,"meitnerium","Mt","unknown","278"),(110,"darmstadtium","Ds","unknown","281"),
        (111,"roentgenium","Rg","unknown","282"),(112,"copernicium","Cn","transition metal","285"),
        (113,"nihonium","Nh","unknown","286"),(114,"flerovium","Fl","post-transition metal","289"),
        (115,"moscovium","Mc","unknown","290"),(116,"livermorium","Lv","unknown","293"),
        (117,"tennessine","Ts","unknown","294"),(118,"oganesson","Og","unknown","294"),
    ]
    facts = []
    for num, name, symbol, category, mass in elements:
        facts.append(f'{name}|is_a|chemical element|1.0')
        facts.append(f'{name}|atomic_number|{num}|1.0')
        facts.append(f'{name}|symbol|{symbol}|1.0')
        facts.append(f'{name}|category|{category}|1.0')
        facts.append(f'{name}|atomic_mass|{mass} amu|1.0')
    return facts

def get_physics():
    facts = []
    forces = [
        ("gravity","weakest fundamental force","infinite range"),
        ("electromagnetism","second strongest force","infinite range"),
        ("strong nuclear force","strongest fundamental force","subatomic range"),
        ("weak nuclear force","third strongest force","subatomic range"),
    ]
    for name, strength, rng in forces:
        facts.append(f'{name}|is_a|fundamental force|1.0')
        facts.append(f'{name}|property|{strength}|1.0')
        facts.append(f'{name}|has|{rng}|1.0')
    particles = [
        ("up quark","quark","2/3 charge"),("down quark","quark","-1/3 charge"),
        ("charm quark","quark","2/3 charge"),("strange quark","quark","-1/3 charge"),
        ("top quark","quark","2/3 charge"),("bottom quark","quark","-1/3 charge"),
        ("electron","lepton","-1 charge"),("muon","lepton","-1 charge"),
        ("tau","lepton","-1 charge"),("electron neutrino","lepton","0 charge"),
        ("muon neutrino","lepton","0 charge"),("tau neutrino","lepton","0 charge"),
        ("photon","boson","force carrier of electromagnetism"),
        ("gluon","boson","force carrier of strong force"),
        ("w boson","boson","force carrier of weak force"),
        ("z boson","boson","force carrier of weak force"),
        ("higgs boson","boson","gives mass to particles"),
    ]
    for name, ptype, prop in particles:
        facts.append(f'{name}|is_a|{ptype}|1.0')
        facts.append(f'{name}|property|{prop}|1.0')
    constants = [
        ("speed of light","299792458 m/s","c"),
        ("gravitational constant","6.674e-11 N m2/kg2","G"),
        ("planck constant","6.626e-34 J s","h"),
        ("boltzmann constant","1.381e-23 J/K","k"),
        ("avogadro number","6.022e23 per mol","Na"),
        ("elementary charge","1.602e-19 C","e"),
        ("vacuum permittivity","8.854e-12 F/m","epsilon_0"),
        ("vacuum permeability","1.257e-6 H/m","mu_0"),
    ]
    for name, value, sym in constants:
        facts.append(f'{name}|equals|{value}|1.0')
        facts.append(f'{name}|symbol|{sym}|1.0')
    return facts

def get_human_body():
    facts = []
    systems = [
        ("circulatory system","transports blood","heart blood vessels"),
        ("respiratory system","gas exchange","lungs trachea bronchi"),
        ("nervous system","signal transmission","brain spinal cord nerves"),
        ("digestive system","breaks down food","stomach intestines liver"),
        ("skeletal system","structural support","bones cartilage ligaments"),
        ("muscular system","movement","skeletal smooth cardiac muscles"),
        ("endocrine system","hormone production","thyroid pituitary adrenal glands"),
        ("immune system","disease defense","white blood cells antibodies lymph"),
        ("integumentary system","protection","skin hair nails"),
        ("urinary system","waste removal","kidneys bladder urethra"),
        ("reproductive system","reproduction","varies by sex"),
        ("lymphatic system","fluid balance","lymph nodes vessels spleen"),
    ]
    for name, function, organs in systems:
        facts.append(f'{name}|is_a|body system|1.0')
        facts.append(f'{name}|function|{function}|1.0')
        facts.append(f'{name}|includes|{organs}|1.0')
    organs = [
        ("heart","pumps blood","300 grams average"),
        ("brain","controls body","1.4 kg average"),
        ("liver","detoxification","largest internal organ"),
        ("lungs","oxygen exchange","pair of organs"),
        ("kidneys","filter blood","pair of bean shaped organs"),
        ("stomach","digests food","holds 1 liter average"),
        ("pancreas","produces insulin","behind stomach"),
        ("spleen","filters blood","left side of abdomen"),
        ("gallbladder","stores bile","under liver"),
        ("thyroid","metabolism regulation","butterfly shaped"),
        ("adrenal glands","stress hormones","above kidneys"),
        ("pituitary gland","master gland","base of brain"),
        ("hypothalamus","homeostasis","in brain"),
        ("skin","largest organ","1.5 to 2 square meters"),
    ]
    for name, function, detail in organs:
        facts.append(f'{name}|function|{function}|1.0')
        facts.append(f'{name}|detail|{detail}|0.9')
    bones = [
        "femur","tibia","fibula","humerus","radius","ulna","skull","spine",
        "pelvis","scapula","clavicle","sternum","ribs","patella","mandible",
    ]
    for bone in bones:
        facts.append(f'{bone}|is_a|bone|1.0')
    return facts

def get_science_facts():
    facts = ['# === PERIODIC TABLE (ALL 118 ELEMENTS) ===']
    facts.extend(get_elements())
    facts.append('')
    facts.append('# === PHYSICS ===')
    facts.extend(get_physics())
    facts.append('')
    facts.append('# === HUMAN BODY SYSTEMS ===')
    facts.extend(get_human_body())
    return facts
