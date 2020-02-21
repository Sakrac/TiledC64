// samples for struse.h

#define STRUSE_IMPLEMENTATION
#include "struse.h"

static const char aSomeFiles[] = {
	"Contents/Resources/ar.lproj/navigation.json\n"
	"Contents/Resources/ar.lproj/redirect.html\n"
	"Contents/Resources/ar.lproj/search.helpindex\n"
	"Contents/Resources/ca.lproj/calc34870.html\n"
	"Contents/Resources/ca.lproj/index.html\n"
	"Contents/Resources/ca.lproj/InfoPlist.strings\n"
	"Contents/Resources/ca.lproj/locale-info.json\n"
	"Contents/Resources/ca.lproj/navigation.json\n"
	"Contents/Resources/ca.lproj/redirect.html\n"
	"Contents/Resources/ca.lproj/search.helpindex\n"
	"Contents/Resources/cs.lproj/calc34870.html\n"
	"Contents/Resources/cs.lproj/index.html\n"
	"Contents/Resources/cs.lproj/InfoPlist.strings\n"
	"Contents/Resources/cs.lproj/locale-info.json\n"
	"Contents/Resources/cs.lproj/navigation.json\n"
	"Contents/Resources/cs.lproj/redirect.html\n"
	"Contents/Resources/cs.lproj/search.helpindex\n"
	"Contents/Resources/da.lproj/calc34870.html\n"
	"Contents/Resources/da.lproj/index.html\n"
	"Contents/Resources/da.lproj/InfoPlist.strings\n"
	"Contents/Resources/da.lproj/locale-info.json\n"
	"Contents/Resources/da.lproj/navigation.json\n"
	"Contents/Resources/da.lproj/redirect.html\n"
	"Contents/Resources/da.lproj/search.helpindex\n"
	"Contents/Resources/de.lproj/calc34870.html\n"
	"Contents/Resources/de.lproj/index.html\n"
	"Contents/Resources/de.lproj/InfoPlist.strings\n"
	"Contents/Resources/de.lproj/locale-info.json\n"
	"Contents/Resources/de.lproj/navigation.json\n"
	"Contents/Resources/de.lproj/redirect.html\n"
	"Contents/Resources/de.lproj/search.helpindex\n"
	"Contents/Resources/el.lproj/calc34870.html\n"
	"Contents/Resources/el.lproj/index.html\n"
	"Contents/Resources/el.lproj/InfoPlist.strings\n"
	"Contents/Resources/el.lproj/locale-info.json\n"
	"Contents/Resources/el.lproj/navigation.json\n"
	"Contents/Resources/el.lproj/redirect.html\n"
	"Contents/Resources/el.lproj/search.helpindex\n"
	"Contents/Resources/en.lproj/calc34870.html\n"
	"Contents/Resources/en.lproj/index.html\n"
	"Contents/Resources/en.lproj/InfoPlist.strings\n"
	"Contents/Resources/en.lproj/locale-info.json\n"
	"Contents/Resources/en.lproj/navigation.json\n"
	"Contents/Resources/en.lproj/redirect.html\n"
	"Contents/Resources/en.lproj/search.helpindex\n"
	"Contents/Resources/es.lproj/calc34870.html\n"
	"Contents/Resources/es.lproj/index.html\n"
	"Contents/Resources/es.lproj/InfoPlist.strings\n"
	"Contents/Resources/es.lproj/locale-info.json\n"
	"Contents/Resources/es.lproj/navigation.json\n"
	"Contents/Resources/es.lproj/redirect.html\n"
	"Contents/Resources/es.lproj/search.helpindex\n"
};


static const char aTextSample[] = {
	"Crime burst in like a flood; modesty, truth, and honor fled. In\n"
	"their places came fraud and cunning, violence, and the wicked\n"
	"love of gain.Then seamen spread sails to the wind, and the\n"
	"trees were torn from the mountains to serve for keels to ships,\n"
	"and vex the face of ocean.The earth, which till now had been\n"
	"cultivated in common, began to be divided off into possessions.\n"
	"Men were not satisfied with what the surface produced, but must\n"
	"dig into its bowels, and draw forth from thence the ores of\n"
	"metals.Mischievous IRON, and more mischievous GOLD, were\n"
	"produced.War sprang up, using both as weapons; the guest was\n"
	"not safe in his friend's house; and sons-in-law and fathers-in-\n"
	"law, brothers and sisters, husbands and wives, could not trust\n"
	"one another.Sons wished their fathers dead, that they might\n"
	"come to the inheritance; family love lay prostrate.The earth\n"
	"was wet with slaughter, and the gods abandoned it, one by one,\n"
	"till Astraea [the goddess of innocence and purity]. After leaving\n"
	"earth, she was placed among the stars, where she became the\n"
	"constellation Virgo The Virgin. Themis(Justice) was the mother\n"
	"of Astraea.She is represented as holding aloft a pair of\n"
	"scales, in which she weighs the claims of opposing parties.It\n"
	"was a favorite idea of the old poets, that these goddesses would\n"
	"one day return, and bring back the Golden Age."
};


bool strref_samples()
{
	// separating full path into name, extension, directory
	strref file_sample("c:\\windows\\system32\\autochk.exe");
	strref file_ext = file_sample.after_last('.');				// file extension
	strref file_name = file_sample.after_last_or_full('\\', '/');	// file name including extension
	strref file_name_no_ext = file_name.before_last('.');		// file name excluding extension
	strref file_dir = file_sample.before_last('\\', '/');		// file path excluding name

	printf("\nfull path: " STRREF_FMT "\nextension: " STRREF_FMT "\nname: " STRREF_FMT
		"\nname no ext: " STRREF_FMT "\ndirectory: " STRREF_FMT "\n\n",
		STRREF_ARG(file_sample), STRREF_ARG(file_ext), STRREF_ARG(file_name),
		STRREF_ARG(file_name_no_ext), STRREF_ARG(file_dir));

	// fnv1a helper
	unsigned int hash = file_sample.fnv1a();
	printf("fnv1a hash of \"" STRREF_FMT "\" is 0x%08x\n", STRREF_ARG(file_sample), hash);

	// find substrings in text
	strref text("Many gnomes don't live underwater, this is a widely appreciated convenience among fish.");

	// case insensitive substring find
	int pos = text.find(strref("THIS"));
	if (pos < 0 || text[pos] != 't')
		return false;

	// find by rolling hash is an option, may be a performance improvement in large text blocks
	pos = text.find_rh_case(strref("Many"));
	if (pos < 0 || text[pos] != 'M')
		return false;

	pos = text.find_rh_case(strref("widely"));
	if (pos < 0 || text[pos] != 'w')
		return false;

	// search character by character, usually performing well
	pos = text.find_case("Many");
	if (pos < 0 || text[pos] != 'M')
		return false;

	// find the position of a secondary search
	pos = text.find_after_last(' ', 'i');
	if (pos < 0 || text[pos] != 'i')
		return false;

	// find last string match
	pos = text.find_last("on");
	if (pos < 0 || text[pos] != 'o')
		return false;

	pos = text.find(',');
	if (pos < 0 || text[pos] != ',')
		return false;

	pos = text.find_after('i', (strl_t)pos);
	if (pos < 0 || (text[pos] != 'i' && pos <= text.find('i')))
		return false;

	pos = (int)text.find_or_full(';', 10);
	if ((strl_t)pos != text.get_len())
		return false;

	pos = (int)text.find_or_full('.', 10);
	if (pos <= 0 || text[pos] != '.')
		return false;

	pos = text.find(strref("this"));
	if (pos < 0 || text[pos] != 't')
		return false;

	pos = text.find("among");
	if (pos < 0 || text[pos] != 'a')
		return false;

	pos = text.find_case("This");
	if (pos >= 0)
		return false;

	pos = text.find_case("this");
	if (pos < 0 || text[pos] != 't')
		return false;

	pos = text.find_last(strref("fi"));
	if (pos < 0 || text[pos] != 'f')
		return false;

	pos = text.find_last("on");
	if (pos < 0 || text[pos] != 'o')
		return false;

	return true;
}



bool wildcard_samples()
{
	// wildcard search examples
	strref search_text("in 2 out of 5 cases the trees will outnumber the carrots, willoutnumber");

	// basic wildcard find substring (*)
	strref search("the*will");
	strref substr = search_text.find_wildcard(search);
	printf("found \"" STRREF_FMT "\" matching \"" STRREF_FMT "\"\n", STRREF_ARG(substr), STRREF_ARG(search));
	if (!substr.same_str_case("the trees will"))
		return false;

	// find word beginning with c
	substr = search_text.find_wildcard("<c*>");
	if (!substr.same_str_case("cases"))
		return false;

	// find word ending with e
	substr = search_text.find_wildcard("<*%e>");
	if (!substr.same_str_case("the"))
		return false;

	// find whole line
	substr = search_text.find_wildcard("@*^");
	if (!substr.same_str_case(search_text))
		return false;

	// using ranges and single character number
	substr = search_text.find_wildcard(strref("[0-9] out of #"));
	if (!substr.same_str_case("2 out of 5"))
		return false;

	// searching any substring between two ranged characters
	substr = search_text.find_wildcard(strref("#*[0-9]"));
	if (!substr.same_str_case("2 out of 5"))
		return false;

	// searching for substring but spaces are not allowed
	substr = search_text.find_wildcard("will*{! }number");
	printf("substr = " STRREF_FMT "\n", STRREF_ARG(substr));
	if (!substr.same_str_case("willoutnumber"))
		return false;

	// find r folloed by numbers without another substring to find
	substr = strref("no k12 r13 [99]").find_wildcard("r*{0-9}");
	if (!substr.same_str_case("r13"))
		return false;

	// search wildcard iteratively
	search = "radio, gorilla, zebra, monkey, human, rat, car, ocelot, conrad, butler";
	substr.clear();
	printf("Words with 'r': ");
	while ((substr = search.wildcard_after("<*$r*$>", substr)))
		printf("\"" STRREF_FMT "\" ", STRREF_ARG(substr));
	printf("\n");

	// search for a more complex expression
	search = strref(aSomeFiles, sizeof(aSomeFiles)-1);
	substr.clear();
	printf("Lines with two subsequent characters d-e (dd, de, ed, ee) and only alphanumeric or dot after:\n");
	while ((substr = search.wildcard_after("@*@[d-e][d-e]*{0-9A-Za-z.}^", substr))) {
		printf("\"" STRREF_FMT "\"\n", STRREF_ARG(substr));
	}
	substr.clear();

	printf("\n.json files:\n\n");
	while ((substr = search.wildcard_after("<*{!/}.json^", substr))) {
		printf("\"" STRREF_FMT "\"\n", STRREF_ARG(substr));
	}

	printf("\nsome words with a character a-f followed by a character that is not a-f:\n\n");
	search = strref(aTextSample, sizeof(aTextSample)-1);
	substr.clear();
	while ((substr = search.wildcard_after("<*$[a-f][!a-f\\1-\\x40]*>", substr))) {
		printf("\"" STRREF_FMT "\" ", STRREF_ARG(substr));
	}
	printf("\n");
	return true;

}





bool strown_samples() {
	// strown insert
	strown<64> str("thougth");
	str.insert("ugh it was never ca", 3);
	printf("\"" STRREF_FMT "\"\n", STRREF_ARG(str.get_strref()));

	// remove all instances of a character
	str.remove('a');
	printf("\"" STRREF_FMT "\"\n", STRREF_ARG(str.get_strref()));

	// remove a substring by position and length
	str.remove(2, 10);
	printf("\"" STRREF_FMT "\"\n", STRREF_ARG(str.get_strref()));

	// copy and trim trailing whitespace
	str.copy("whitespaces:   ");
	str.clip_trailing_whitespace();
	if (str.get_last()!=':')
		return false;

	// find whitespace
	str.copy("one two three");
	if (str.find_whitespace() != 3 || str.find_whitespace_or_full() != 3)
		return false;

	// word size count
	if (str.len_word() != 3)
		return false;

	// alphanumeric count
	str.copy("1701 green");
	if (str.len_alphanumeric() != 4)
		return false;

	// uppercase
	str.copy("AbCdEF");
	str.toupper();
	if (!str.same_str_case("ABCDEF"))
		return false;

	// lowercase
	str.tolower();
	if (!str.same_str_case("abcdef"))
		return false;

	// copy a substring within a string
	str.copy("0123456789abcdefghijklmnopqrstuvwxyz");
	str.substrcopy(5, 1, 10);
	if (!str.same_str_case("056789abcdebcdefghijklmnopqrstuvwxyz"))
		return false;

	// format with {} notation
	strref aArgs[] = {
		strref("test argument 0"),
		strref("sample param 1")
	};
	str.format("\\t\\{1} is {1}\\n\\t\\{0} is {0}\\n", aArgs);
	printf(STRREF_FMT, STRREF_ARG(str.get_strref()));

	// sprintf with standard c style variadic printf rules
	str.sprintf("any %.2f horse is mean.", 0.5f);
	str.sprintf_append(" which is complete %s\n", "horse");
	printf(STRREF_FMT, STRREF_ARG(str));

	return true;
}

int main(int argc, char **argv) {
	(void)argc;
	(void)argv;
    if (!strref_samples())
        printf("strref sample failed\n");
	if (!wildcard_samples())
		printf("wildcard sample failed\n");
	if (!strown_samples())
        printf("strown sample failed\n");
    return 0;
}
