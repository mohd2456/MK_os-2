# MK Lexicon - Word Relationships
# Format: word|type|related_words (comma-separated)
# Types: synonym, antonym, plural, verb_past, verb_present, verb_future,
#        verb_gerund, adjective_comparative, adjective_superlative, category
# Used by the Linguistic Engine for synonym expansion, conjugation, and variation.

# === SYNONYMS ===
big|synonym|large,huge,massive,enormous,giant,vast
small|synonym|tiny,little,miniature,compact,petite,minute
fast|synonym|quick,rapid,speedy,swift,hasty,brisk
slow|synonym|sluggish,gradual,leisurely,unhurried,plodding
good|synonym|great,excellent,wonderful,fine,superb,outstanding
bad|synonym|terrible,awful,poor,dreadful,horrible,lousy
happy|synonym|glad,joyful,cheerful,delighted,pleased,content
sad|synonym|unhappy,sorrowful,gloomy,melancholy,downcast,dejected
smart|synonym|intelligent,clever,brilliant,wise,sharp,bright
easy|synonym|simple,straightforward,effortless,basic,uncomplicated
hard|synonym|difficult,tough,challenging,complex,demanding
important|synonym|crucial,vital,essential,significant,critical,key
beautiful|synonym|gorgeous,stunning,lovely,attractive,elegant
new|synonym|fresh,recent,modern,novel,latest,current
old|synonym|ancient,aged,vintage,antique,elderly,mature
think|synonym|consider,ponder,reflect,contemplate,reason,analyze
make|synonym|create,build,construct,produce,craft,assemble
help|synonym|assist,aid,support,facilitate,enable
say|synonym|state,mention,express,declare,articulate,convey
know|synonym|understand,comprehend,recognize,realize,grasp
like|synonym|enjoy,appreciate,prefer,favor,admire,fancy
want|synonym|desire,wish,need,crave,seek,require
use|synonym|utilize,employ,apply,leverage,operate,wield
show|synonym|display,demonstrate,reveal,present,exhibit
find|synonym|discover,locate,identify,detect,uncover,spot

# === ANTONYMS ===
big|antonym|small,tiny,little,miniature
fast|antonym|slow,sluggish,gradual
good|antonym|bad,terrible,awful,poor
happy|antonym|sad,unhappy,miserable
hot|antonym|cold,cool,freezing,chilly
light|antonym|dark,heavy
up|antonym|down
left|antonym|right
open|antonym|closed,shut
start|antonym|stop,end,finish
true|antonym|false
yes|antonym|no
easy|antonym|hard,difficult
new|antonym|old,ancient
strong|antonym|weak,fragile
loud|antonym|quiet,silent
full|antonym|empty
win|antonym|lose
love|antonym|hate
push|antonym|pull
give|antonym|take
buy|antonym|sell
create|antonym|destroy,delete

# === PLURALS ===
cat|plural|cats
dog|plural|dogs
file|plural|files
class|plural|classes
process|plural|processes
function|plural|functions
array|plural|arrays
system|plural|systems
person|plural|people
child|plural|children
mouse|plural|mice
datum|plural|data
index|plural|indices
analysis|plural|analyses
criterion|plural|criteria
phenomenon|plural|phenomena
matrix|plural|matrices
vertex|plural|vertices
bus|plural|buses
box|plural|boxes

# === VERB FORMS: PAST TENSE ===
run|verb_past|ran
make|verb_past|made
build|verb_past|built
create|verb_past|created
write|verb_past|wrote
read|verb_past|read
think|verb_past|thought
know|verb_past|knew
find|verb_past|found
give|verb_past|gave
take|verb_past|took
see|verb_past|saw
go|verb_past|went
come|verb_past|came
get|verb_past|got
have|verb_past|had
do|verb_past|did
say|verb_past|said
use|verb_past|used
try|verb_past|tried
start|verb_past|started
stop|verb_past|stopped
work|verb_past|worked
help|verb_past|helped
show|verb_past|showed
learn|verb_past|learned
teach|verb_past|taught
break|verb_past|broke
fix|verb_past|fixed
send|verb_past|sent

# === VERB FORMS: PRESENT (third person singular) ===
run|verb_present|runs
make|verb_present|makes
build|verb_present|builds
create|verb_present|creates
write|verb_present|writes
read|verb_present|reads
think|verb_present|thinks
know|verb_present|knows
find|verb_present|finds
give|verb_present|gives
take|verb_present|takes
see|verb_present|sees
go|verb_present|goes
come|verb_present|comes
get|verb_present|gets
have|verb_present|has
do|verb_present|does
say|verb_present|says
use|verb_present|uses
try|verb_present|tries

# === VERB FORMS: GERUND (-ing) ===
run|verb_gerund|running
make|verb_gerund|making
build|verb_gerund|building
create|verb_gerund|creating
write|verb_gerund|writing
read|verb_gerund|reading
think|verb_gerund|thinking
know|verb_gerund|knowing
find|verb_gerund|finding
give|verb_gerund|giving
take|verb_gerund|taking
see|verb_gerund|seeing
go|verb_gerund|going
come|verb_gerund|coming
get|verb_gerund|getting
have|verb_gerund|having
do|verb_gerund|doing
say|verb_gerund|saying
use|verb_gerund|using
try|verb_gerund|trying
work|verb_gerund|working
help|verb_gerund|helping
learn|verb_gerund|learning
start|verb_gerund|starting
stop|verb_gerund|stopping

# === ADJECTIVE COMPARATIVE ===
big|adjective_comparative|bigger
small|adjective_comparative|smaller
fast|adjective_comparative|faster
slow|adjective_comparative|slower
good|adjective_comparative|better
bad|adjective_comparative|worse
hot|adjective_comparative|hotter
cold|adjective_comparative|colder
hard|adjective_comparative|harder
easy|adjective_comparative|easier
strong|adjective_comparative|stronger
weak|adjective_comparative|weaker
old|adjective_comparative|older
new|adjective_comparative|newer
long|adjective_comparative|longer
short|adjective_comparative|shorter

# === ADJECTIVE SUPERLATIVE ===
big|adjective_superlative|biggest
small|adjective_superlative|smallest
fast|adjective_superlative|fastest
slow|adjective_superlative|slowest
good|adjective_superlative|best
bad|adjective_superlative|worst
hot|adjective_superlative|hottest
cold|adjective_superlative|coldest
hard|adjective_superlative|hardest
easy|adjective_superlative|easiest
strong|adjective_superlative|strongest
old|adjective_superlative|oldest
new|adjective_superlative|newest
long|adjective_superlative|longest
short|adjective_superlative|shortest

# === CATEGORY MEMBERSHIP (word belongs to category) ===
python|category|programming_language
cpp|category|programming_language
javascript|category|programming_language
html|category|markup_language
css|category|styling_language
function|category|programming_concept
class|category|programming_concept
variable|category|programming_concept
loop|category|programming_concept
array|category|data_structure
list|category|data_structure
map|category|data_structure
tree|category|data_structure
graph|category|data_structure
stack|category|data_structure
queue|category|data_structure
sort|category|algorithm
search|category|algorithm
if|category|control_flow
while|category|control_flow
for|category|control_flow
return|category|control_flow

# === IS/ARE AGREEMENT ===
i|verb_agreement|am
you|verb_agreement|are
he|verb_agreement|is
she|verb_agreement|is
it|verb_agreement|is
we|verb_agreement|are
they|verb_agreement|are
this|verb_agreement|is
that|verb_agreement|is
these|verb_agreement|are
those|verb_agreement|are

# === HAS/HAVE AGREEMENT ===
i|have_agreement|have
you|have_agreement|have
he|have_agreement|has
she|have_agreement|has
it|have_agreement|has
we|have_agreement|have
they|have_agreement|have
