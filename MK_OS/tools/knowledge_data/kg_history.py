"""History facts: historical events, famous scientists, civilizations."""

def get_historical_events():
    events = [
        ("ancient egypt unified","3100 BC","africa"),
        ("construction of great pyramid","2560 BC","egypt"),
        ("code of hammurabi","1754 BC","babylon"),
        ("trojan war","1200 BC","greece"),
        ("founding of rome","753 BC","italy"),
        ("birth of democracy","508 BC","athens"),
        ("alexander conquers persia","331 BC","middle east"),
        ("fall of roman republic","27 BC","rome"),
        ("birth of jesus christ","approximately 4 BC","judea"),
        ("fall of western roman empire","476 AD","europe"),
        ("rise of islam","622 AD","arabia"),
        ("viking age begins","793 AD","scandinavia"),
        ("norman conquest of england","1066","england"),
        ("magna carta signed","1215","england"),
        ("black death pandemic","1347-1351","europe"),
        ("fall of constantinople","1453","turkey"),
        ("gutenberg printing press","1440","germany"),
        ("columbus reaches americas","1492","caribbean"),
        ("protestant reformation","1517","germany"),
        ("spanish armada defeated","1588","english channel"),
        ("jamestown colony founded","1607","north america"),
        ("thirty years war","1618-1648","europe"),
        ("english civil war","1642-1651","england"),
        ("treaty of westphalia","1648","europe"),
        ("glorious revolution","1688","england"),
        ("american declaration of independence","1776","united states"),
        ("french revolution begins","1789","france"),
        ("haitian revolution","1791-1804","haiti"),
        ("napoleon becomes emperor","1804","france"),
        ("battle of waterloo","1815","belgium"),
        ("greek independence","1821","greece"),
        ("abolition of slavery in british empire","1833","united kingdom"),
        ("communist manifesto published","1848","europe"),
        ("american civil war","1861-1865","united states"),
        ("abolition of slavery in usa","1865","united states"),
        ("meiji restoration","1868","japan"),
        ("opening of suez canal","1869","egypt"),
        ("unification of germany","1871","germany"),
        ("unification of italy","1871","italy"),
        ("spanish american war","1898","americas"),
        ("russian revolution","1917","russia"),
        ("treaty of versailles","1919","france"),
        ("great depression begins","1929","worldwide"),
        ("world war 2 begins","1939","europe"),
        ("united nations founded","1945","worldwide"),
        ("indian independence","1947","india"),
        ("state of israel established","1948","middle east"),
        ("chinese communist revolution","1949","china"),
        ("korean war","1950-1953","korea"),
        ("cuban revolution","1959","cuba"),
        ("berlin wall built","1961","germany"),
        ("cuban missile crisis","1962","cuba"),
        ("moon landing","1969","space"),
        ("vietnam war ends","1975","vietnam"),
        ("fall of berlin wall","1989","germany"),
        ("dissolution of soviet union","1991","russia"),
        ("apartheid ends","1994","south africa"),
        ("european union formed","1993","europe"),
        ("september 11 attacks","2001","united states"),
        ("arab spring","2011","middle east"),
    ]
    facts = []
    for event, date, location in events:
        facts.append(f'{event}|date|{date}|1.0')
        facts.append(f'{event}|location|{location}|1.0')
        facts.append(f'{event}|is_a|historical event|1.0')
    return facts

def get_scientists():
    scientists = [
        ("isaac newton","physics mathematics","laws of motion and universal gravitation","1643-1727"),
        ("albert einstein","physics","theory of relativity","1879-1955"),
        ("galileo galilei","physics astronomy","telescopic observations heliocentric support","1564-1642"),
        ("nikola tesla","electrical engineering","alternating current system","1856-1943"),
        ("marie curie","physics chemistry","radioactivity research","1867-1934"),
        ("charles darwin","biology","theory of evolution by natural selection","1809-1882"),
        ("louis pasteur","microbiology chemistry","germ theory pasteurization","1822-1895"),
        ("michael faraday","physics chemistry","electromagnetic induction","1791-1867"),
        ("james clerk maxwell","physics","electromagnetic theory","1831-1879"),
        ("max planck","physics","quantum theory","1858-1947"),
        ("niels bohr","physics","atomic model","1885-1962"),
        ("erwin schrodinger","physics","wave mechanics","1887-1961"),
        ("werner heisenberg","physics","uncertainty principle","1901-1976"),
        ("richard feynman","physics","quantum electrodynamics","1918-1988"),
        ("stephen hawking","physics cosmology","black hole radiation","1942-2018"),
        ("alexander fleming","medicine","discovery of penicillin","1881-1955"),
        ("gregor mendel","genetics","laws of inheritance","1822-1884"),
        ("james watson","molecular biology","dna structure","1928-present"),
        ("francis crick","molecular biology","dna structure","1916-2004"),
        ("rosalind franklin","chemistry","x-ray crystallography of dna","1920-1958"),
        ("dmitri mendeleev","chemistry","periodic table","1834-1907"),
        ("linus pauling","chemistry","chemical bonding","1901-1994"),
        ("alan turing","mathematics computer science","turing machine and computation theory","1912-1954"),
        ("carl gauss","mathematics","number theory and statistics","1777-1855"),
        ("leonhard euler","mathematics","graph theory and analysis","1707-1783"),
        ("archimedes","mathematics physics","buoyancy and levers","287-212 BC"),
        ("pythagoras","mathematics","pythagorean theorem","570-495 BC"),
        ("euclid","mathematics","elements of geometry","325-265 BC"),
        ("copernicus","astronomy","heliocentric model","1473-1543"),
        ("johannes kepler","astronomy","laws of planetary motion","1571-1630"),
        ("edwin hubble","astronomy","expanding universe","1889-1953"),
        ("carl linnaeus","biology","taxonomy classification system","1707-1778"),
        ("antoine lavoisier","chemistry","modern chemistry founder","1743-1794"),
        ("robert hooke","physics biology","cell discovery and hooke law","1635-1703"),
        ("alexander graham bell","engineering","telephone invention","1847-1922"),
        ("thomas edison","engineering","electric light bulb and phonograph","1847-1931"),
        ("wright brothers","engineering","powered flight","1867-1948"),
        ("enrico fermi","physics","nuclear reactor","1901-1954"),
        ("ernest rutherford","physics","nuclear physics founder","1871-1937"),
        ("paul dirac","physics","quantum mechanics and antimatter","1902-1984"),
    ]
    facts = []
    for name, field, contribution, years in scientists:
        facts.append(f'{name}|is_a|scientist|1.0')
        facts.append(f'{name}|field|{field}|1.0')
        facts.append(f'{name}|contribution|{contribution}|1.0')
        facts.append(f'{name}|lived|{years}|1.0')
    return facts

def get_civilizations():
    civs = [
        ("ancient egypt","3100 BC to 30 BC","nile river valley","pyramids hieroglyphics"),
        ("mesopotamia","3500 BC to 539 BC","tigris euphrates","cuneiform writing wheel"),
        ("indus valley","3300 BC to 1300 BC","indus river","urban planning drainage"),
        ("ancient china","2070 BC to 221 BC","yellow river","silk paper gunpowder"),
        ("ancient greece","800 BC to 146 BC","mediterranean","democracy philosophy"),
        ("roman empire","27 BC to 476 AD","mediterranean","law engineering roads"),
        ("persian empire","550 BC to 330 BC","middle east","postal system tolerance"),
        ("maya civilization","2000 BC to 1500 AD","central america","calendar mathematics"),
        ("aztec empire","1300 to 1521","mexico","chinampas architecture"),
        ("inca empire","1438 to 1533","south america","roads quipu system"),
        ("byzantine empire","330 to 1453","eastern mediterranean","preserved roman knowledge"),
        ("ottoman empire","1299 to 1922","middle east europe","multicultural administration"),
        ("mongol empire","1206 to 1368","central asia","largest land empire"),
        ("han dynasty","206 BC to 220 AD","china","silk road paper"),
        ("tang dynasty","618 to 907","china","golden age poetry"),
        ("mughal empire","1526 to 1857","south asia","taj mahal architecture"),
    ]
    facts = []
    for name, period, region, achievements in civs:
        facts.append(f'{name}|is_a|civilization|1.0')
        facts.append(f'{name}|period|{period}|1.0')
        facts.append(f'{name}|region|{region}|1.0')
        facts.append(f'{name}|achievements|{achievements}|1.0')
    return facts

def get_history_facts():
    facts = ['# === HISTORICAL EVENTS ===']
    facts.extend(get_historical_events())
    facts.append('')
    facts.append('# === FAMOUS SCIENTISTS ===')
    facts.extend(get_scientists())
    facts.append('')
    facts.append('# === CIVILIZATIONS ===')
    facts.extend(get_civilizations())
    return facts
