//
// SAMPLE USE OF STRREF XML CALLBACK PARSER
//



#define STRUSE_IMPLEMENTATION
#include "struse.h"
#include "xml.h"

#include <vector>
#include <string>

const char aXMLExample[] = {
	"<?xml version=\"1.0\"?>\n"
	"<root>\n"
	"  <sprite type=\"billboard\">\n"
	"    <color red=\"32\" blue='192' green=\"255\"/>"
	"    <bitmap>textures/yourface.dds</bitmap>\n"
	"    <doublesided/>"
	"    <size width='128' height='128'/>"
	"  </sprite>\n"
	"  <sprite type=\"static\">\n"
	"    <color red=\"32\" green=\"64\" blue='255'/>"
	"    <bitmap>textures/splash.dds</bitmap>\n"
	"    <size width='32' height='32'/>"
	"  </sprite>\n"
	"</root>\n"
};

#define PHASH(name, hash) (hash)

#define SPRITE_TAG_SPRITE PHASH("sprite", 0x81e2581c)
#define SPRITE_TAG_COLOR PHASH("color", 0x3d7e6258)
#define SPRITE_TAG_BITMAP PHASH("bitmap", 0x46544626)
#define SPRITE_TAG_DOUBLESIDED PHASH("doublesided", 0x06216465)
#define SPRITE_TAG_SIZE PHASH("size", 0x23a0d95c)
#define SPRITE_SPRITE_TYPE PHASH("type", 0x5127f14d)
#define SPRITE_COLOR_RED PHASH("red", 0x40f480dc)
#define SPRITE_COLOR_GREEN PHASH("green", 0x011decbc)
#define SPRITE_COLOR_BLUE PHASH("blue", 0x82fbf5cd)
#define SPRITE_SIZE_WIDTH PHASH("width", 0x95876e1f)
#define SPRITE_SIZE_HEIGHT PHASH("height", 0xd5bdbb42)

//typedef bool(*XMLDataCB)(void* /*user*/, strref /*tag_or_data*/, const strref* /*tag_stack*/, int /*depth*/, XML_TYPE /*type*/);

struct Sprite {
	unsigned char red, green, blue;
	strown<128> bitmap;
	bool doublesided;
	int width, height;

	Sprite() : red(255), green(255), blue(255), doublesided(false), width(0), height(0) {}
};

class SpriteList {
public:
	std::vector<Sprite> sprites;
};

struct ParseSpriteList {
	SpriteList *pSprites;
};

bool XMLExample_Callback(void *user, strref element, const strref *stack, int depth, XML_TYPE type)
{
	SpriteList *pSprites = (SpriteList*)user;

	// what sprite is currently being parsed?
	size_t count = pSprites->sprites.size();
	size_t curr = count ? count - 1 : 0;

	if (type == XML_TYPE_TEXT) {
		// this is text between tags, check the parent tag by looking at the
		// most recent element on the tag stack! note: may be called without depth (outside all tags)
		if (depth && stack[0].get_word_ws().fnv1a() == SPRITE_TAG_BITMAP) {
			pSprites->sprites[curr].bitmap.copy(element);
		}
	} else if (type == XML_TYPE_TAG_OPEN || type == XML_TYPE_TAG_SELF_CLOSE) {
		strref tag = element.get_word_ws();
		switch (tag.fnv1a()) {
			case SPRITE_TAG_SPRITE:
				pSprites->sprites.push_back(Sprite());
				break;
			case SPRITE_TAG_COLOR:
				if (!count)		// can't have a color tag outside of a sprite tag
					return false;
				for (strref attr = XMLFirstAttribute(element); attr; attr = XMLNextAttribute(attr)) {
					switch (XMLAttributeName(attr).fnv1a()) {
						case SPRITE_COLOR_RED:
							pSprites->sprites[curr].red = (unsigned char)XMLAttributeValue(attr).atoi();
							break;
						case SPRITE_COLOR_GREEN:
							pSprites->sprites[curr].green = (unsigned char)XMLAttributeValue(attr).atoi();
							break;
						case SPRITE_COLOR_BLUE:
							pSprites->sprites[curr].blue = (unsigned char)XMLAttributeValue(attr).atoi();
							break;
					}
				}
				break;
			case SPRITE_TAG_DOUBLESIDED:
				if (!count)		// can't have a doulesided tag outside of a sprite tag
					return false;
				pSprites->sprites[curr].doublesided = true;
				break;
			case SPRITE_TAG_SIZE:
				if (!count)		// can't have a size tag outside of a sprite tag
					return false;
				for (strref attr = XMLFirstAttribute(element); attr; attr = XMLNextAttribute(attr)) {
					switch (XMLAttributeName(attr).fnv1a()) {
					case SPRITE_SIZE_WIDTH:
						pSprites->sprites[curr].width = (int)XMLAttributeValue(attr).atoi();
						break;
					case SPRITE_SIZE_HEIGHT:
						pSprites->sprites[curr].height = (int)XMLAttributeValue(attr).atoi();
						break;
					}
				}
				break;
		}
	}

	return true;
}


int main(int argc, char **argv) {
	SpriteList sprites;

	ParseXML(strref(aXMLExample, sizeof(aXMLExample) - 1), XMLExample_Callback, &sprites);

	return 0;
}
