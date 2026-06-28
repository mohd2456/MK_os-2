"""Geography facts: 195+ countries with capitals and continents, oceans, mountains."""

def get_countries():
    """All 195 UN-recognized countries with capitals and continents."""
    countries = [
        ("afghanistan","kabul","asia"),("albania","tirana","europe"),
        ("algeria","algiers","africa"),("andorra","andorra la vella","europe"),
        ("angola","luanda","africa"),("antigua and barbuda","saint johns","north america"),
        ("argentina","buenos aires","south america"),("armenia","yerevan","asia"),
        ("australia","canberra","oceania"),("austria","vienna","europe"),
        ("azerbaijan","baku","asia"),("bahamas","nassau","north america"),
        ("bahrain","manama","asia"),("bangladesh","dhaka","asia"),
        ("barbados","bridgetown","north america"),("belarus","minsk","europe"),
        ("belgium","brussels","europe"),("belize","belmopan","north america"),
        ("benin","porto-novo","africa"),("bhutan","thimphu","asia"),
        ("bolivia","sucre","south america"),("bosnia and herzegovina","sarajevo","europe"),
        ("botswana","gaborone","africa"),("brazil","brasilia","south america"),
        ("brunei","bandar seri begawan","asia"),("bulgaria","sofia","europe"),
        ("burkina faso","ouagadougou","africa"),("burundi","gitega","africa"),
        ("cabo verde","praia","africa"),("cambodia","phnom penh","asia"),
        ("cameroon","yaounde","africa"),("canada","ottawa","north america"),
        ("central african republic","bangui","africa"),("chad","ndjamena","africa"),
        ("chile","santiago","south america"),("china","beijing","asia"),
        ("colombia","bogota","south america"),("comoros","moroni","africa"),
        ("congo republic","brazzaville","africa"),("costa rica","san jose","north america"),
        ("croatia","zagreb","europe"),("cuba","havana","north america"),
        ("cyprus","nicosia","europe"),("czech republic","prague","europe"),
        ("democratic republic of congo","kinshasa","africa"),("denmark","copenhagen","europe"),
        ("djibouti","djibouti city","africa"),("dominica","roseau","north america"),
        ("dominican republic","santo domingo","north america"),("east timor","dili","asia"),
        ("ecuador","quito","south america"),("egypt","cairo","africa"),
        ("el salvador","san salvador","north america"),("equatorial guinea","malabo","africa"),
        ("eritrea","asmara","africa"),("estonia","tallinn","europe"),
        ("eswatini","mbabane","africa"),("ethiopia","addis ababa","africa"),
        ("fiji","suva","oceania"),("finland","helsinki","europe"),
        ("france","paris","europe"),("gabon","libreville","africa"),
        ("gambia","banjul","africa"),("georgia","tbilisi","asia"),
        ("germany","berlin","europe"),("ghana","accra","africa"),
        ("greece","athens","europe"),("grenada","saint georges","north america"),
        ("guatemala","guatemala city","north america"),("guinea","conakry","africa"),
        ("guinea-bissau","bissau","africa"),("guyana","georgetown","south america"),
        ("haiti","port-au-prince","north america"),("honduras","tegucigalpa","north america"),
        ("hungary","budapest","europe"),("iceland","reykjavik","europe"),
        ("india","new delhi","asia"),("indonesia","jakarta","asia"),
        ("iran","tehran","asia"),("iraq","baghdad","asia"),
        ("ireland","dublin","europe"),("israel","jerusalem","asia"),
        ("italy","rome","europe"),("ivory coast","yamoussoukro","africa"),
        ("jamaica","kingston","north america"),("japan","tokyo","asia"),
        ("jordan","amman","asia"),("kazakhstan","astana","asia"),
        ("kenya","nairobi","africa"),("kiribati","tarawa","oceania"),
        ("kosovo","pristina","europe"),("kuwait","kuwait city","asia"),
        ("kyrgyzstan","bishkek","asia"),("laos","vientiane","asia"),
        ("latvia","riga","europe"),("lebanon","beirut","asia"),
        ("lesotho","maseru","africa"),("liberia","monrovia","africa"),
        ("libya","tripoli","africa"),("liechtenstein","vaduz","europe"),
        ("lithuania","vilnius","europe"),("luxembourg","luxembourg city","europe"),
        ("madagascar","antananarivo","africa"),("malawi","lilongwe","africa"),
        ("malaysia","kuala lumpur","asia"),("maldives","male","asia"),
        ("mali","bamako","africa"),("malta","valletta","europe"),
        ("marshall islands","majuro","oceania"),("mauritania","nouakchott","africa"),
        ("mauritius","port louis","africa"),("mexico","mexico city","north america"),
        ("micronesia","palikir","oceania"),("moldova","chisinau","europe"),
        ("monaco","monaco","europe"),("mongolia","ulaanbaatar","asia"),
        ("montenegro","podgorica","europe"),("morocco","rabat","africa"),
        ("mozambique","maputo","africa"),("myanmar","naypyidaw","asia"),
        ("namibia","windhoek","africa"),("nauru","yaren","oceania"),
        ("nepal","kathmandu","asia"),("netherlands","amsterdam","europe"),
        ("new zealand","wellington","oceania"),("nicaragua","managua","north america"),
        ("niger","niamey","africa"),("nigeria","abuja","africa"),
        ("north korea","pyongyang","asia"),("north macedonia","skopje","europe"),
        ("norway","oslo","europe"),("oman","muscat","asia"),
        ("pakistan","islamabad","asia"),("palau","ngerulmud","oceania"),
        ("palestine","ramallah","asia"),("panama","panama city","north america"),
        ("papua new guinea","port moresby","oceania"),("paraguay","asuncion","south america"),
        ("peru","lima","south america"),("philippines","manila","asia"),
        ("poland","warsaw","europe"),("portugal","lisbon","europe"),
        ("qatar","doha","asia"),("romania","bucharest","europe"),
        ("russia","moscow","europe"),("rwanda","kigali","africa"),
        ("saint kitts and nevis","basseterre","north america"),
        ("saint lucia","castries","north america"),
        ("saint vincent and the grenadines","kingstown","north america"),
        ("samoa","apia","oceania"),("san marino","san marino","europe"),
        ("sao tome and principe","sao tome","africa"),
        ("saudi arabia","riyadh","asia"),("senegal","dakar","africa"),
        ("serbia","belgrade","europe"),("seychelles","victoria","africa"),
        ("sierra leone","freetown","africa"),("singapore","singapore","asia"),
        ("slovakia","bratislava","europe"),("slovenia","ljubljana","europe"),
        ("solomon islands","honiara","oceania"),("somalia","mogadishu","africa"),
        ("south africa","pretoria","africa"),("south korea","seoul","asia"),
        ("south sudan","juba","africa"),("spain","madrid","europe"),
        ("sri lanka","colombo","asia"),("sudan","khartoum","africa"),
        ("suriname","paramaribo","south america"),("sweden","stockholm","europe"),
        ("switzerland","bern","europe"),("syria","damascus","asia"),
        ("tajikistan","dushanbe","asia"),("tanzania","dodoma","africa"),
        ("thailand","bangkok","asia"),("togo","lome","africa"),
        ("tonga","nukualofa","oceania"),("trinidad and tobago","port of spain","north america"),
        ("tunisia","tunis","africa"),("turkey","ankara","asia"),
        ("turkmenistan","ashgabat","asia"),("tuvalu","funafuti","oceania"),
        ("uganda","kampala","africa"),("ukraine","kyiv","europe"),
        ("united arab emirates","abu dhabi","asia"),("united kingdom","london","europe"),
        ("united states","washington dc","north america"),("uruguay","montevideo","south america"),
        ("uzbekistan","tashkent","asia"),("vanuatu","port vila","oceania"),
        ("vatican city","vatican city","europe"),("venezuela","caracas","south america"),
        ("vietnam","hanoi","asia"),("yemen","sanaa","asia"),
        ("zambia","lusaka","africa"),("zimbabwe","harare","africa"),
    ]
    facts = []
    for country, capital, continent in countries:
        facts.append(f'{country}|is_a|country|1.0')
        facts.append(f'{country}|capital|{capital}|1.0')
        facts.append(f'{country}|continent|{continent}|1.0')
    return facts

def get_geographic_features():
    facts = []
    mountains = [
        ("mount everest","8849 m","asia"),("k2","8611 m","asia"),
        ("kangchenjunga","8586 m","asia"),("lhotse","8516 m","asia"),
        ("makalu","8485 m","asia"),("cho oyu","8188 m","asia"),
        ("dhaulagiri","8167 m","asia"),("manaslu","8163 m","asia"),
        ("nanga parbat","8126 m","asia"),("annapurna","8091 m","asia"),
        ("mount kilimanjaro","5895 m","africa"),("mount elbrus","5642 m","europe"),
        ("mount denali","6190 m","north america"),("aconcagua","6961 m","south america"),
        ("mont blanc","4809 m","europe"),("mount fuji","3776 m","asia"),
        ("mount olympus","2917 m","europe"),("mount rainier","4392 m","north america"),
        ("matterhorn","4478 m","europe"),("mount vesuvius","1281 m","europe"),
    ]
    for name, height, continent in mountains:
        facts.append(f'{name}|is_a|mountain|1.0')
        facts.append(f'{name}|height|{height}|1.0')
        facts.append(f'{name}|located_in|{continent}|1.0')
    rivers = [
        ("nile","6650 km","africa"),("amazon","6400 km","south america"),
        ("yangtze","6300 km","asia"),("mississippi","6275 km","north america"),
        ("yenisei","5539 km","asia"),("yellow river","5464 km","asia"),
        ("ob river","5410 km","asia"),("parana","4880 km","south america"),
        ("congo","4700 km","africa"),("amur","4444 km","asia"),
        ("lena","4400 km","asia"),("mekong","4350 km","asia"),
        ("mackenzie","4241 km","north america"),("niger","4200 km","africa"),
        ("murray","2508 km","oceania"),("danube","2850 km","europe"),
        ("volga","3531 km","europe"),("rhine","1230 km","europe"),
        ("ganges","2525 km","asia"),("indus","3180 km","asia"),
    ]
    for name, length, continent in rivers:
        facts.append(f'{name}|is_a|river|1.0')
        facts.append(f'{name}|length|{length}|1.0')
        facts.append(f'{name}|located_in|{continent}|1.0')
    oceans = [
        ("pacific ocean","largest ocean","165.25 million sq km"),
        ("atlantic ocean","second largest ocean","106.46 million sq km"),
        ("indian ocean","third largest ocean","70.56 million sq km"),
        ("southern ocean","fourth largest ocean","21.96 million sq km"),
        ("arctic ocean","smallest ocean","14.06 million sq km"),
    ]
    for name, rank, area in oceans:
        facts.append(f'{name}|is_a|ocean|1.0')
        facts.append(f'{name}|rank|{rank}|1.0')
        facts.append(f'{name}|area|{area}|1.0')
    deserts = [
        ("sahara","largest hot desert","9.2 million sq km"),
        ("arabian desert","second largest hot desert","2.3 million sq km"),
        ("gobi desert","largest cold desert in asia","1.3 million sq km"),
        ("kalahari desert","southern african desert","900000 sq km"),
        ("patagonian desert","south american desert","673000 sq km"),
        ("great victoria desert","australian desert","647000 sq km"),
        ("syrian desert","middle eastern desert","500000 sq km"),
        ("great basin desert","north american cold desert","492000 sq km"),
        ("chihuahuan desert","north american desert","362000 sq km"),
        ("mojave desert","north american desert","124000 sq km"),
    ]
    for name, desc, area in deserts:
        facts.append(f'{name}|is_a|desert|1.0')
        facts.append(f'{name}|description|{desc}|1.0')
        facts.append(f'{name}|area|{area}|0.9')
    return facts

def get_geography_facts():
    facts = ['# === COUNTRIES AND CAPITALS (195) ===']
    facts.extend(get_countries())
    facts.append('')
    facts.append('# === GEOGRAPHIC FEATURES ===')
    facts.extend(get_geographic_features())
    return facts
