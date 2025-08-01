/****************************************************************************
 *               truetype.cpp
 *
 * This module implements rendering of TrueType fonts.
 * This file was written by Alexander Enzmann.  He wrote the code for
 * rendering glyphs and generously provided us these enhancements.
 *
 * from Persistence of Vision(tm) Ray Tracer version 3.6.
 * Copyright 1991-2003 Persistence of Vision Team
 * Copyright 2003-2004 Persistence of Vision Raytracer Pty. Ltd.
 *---------------------------------------------------------------------------
 * NOTICE: This source code file is provided so that users may experiment
 * with enhancements to POV-Ray and to port the software to platforms other
 * than those supported by the POV-Ray developers. There are strict rules
 * regarding how you are permitted to use this file. These rules are contained
 * in the distribution and derivative versions licenses which should have been
 * provided with this file.
 *
 * These licences may be found online, linked from the end-user license
 * agreement that is located at http://www.povray.org/povlegal.html
 *---------------------------------------------------------------------------
 * This program is based on the popular DKB raytracer version 2.12.
 * DKBTrace was originally written by David K. Buck.
 * DKBTrace Ver 2.0-2.12 were written by David K. Buck & Aaron A. Collins.
 *---------------------------------------------------------------------------
 * $File: //depot/povray/3.6-release/source/truetype.cpp $
 * $Revision: #3 $
 * $Change: 3032 $
 * $DateTime: 2004/08/02 18:43:41 $
 * $Author: chrisc $
 * $Log$
 *****************************************************************************/

#include "frame.h"
#include "povray.h"
#include "vector.h"
#include "bbox.h"
#include "matrices.h"
#include "objects.h"
#include "truetype.h"
#include "csg.h"                /* [ARE 11/94] */
#include "povray.h"
#include "pov_util.h"

BEGIN_POV_NAMESPACE

/*****************************************************************************
* Local preprocessor defines
******************************************************************************/

/* uncomment this to debug ttf. DEBUG1 gives less output than DEBUG2
#define TTF_DEBUG2 1
#define TTF_DEBUG 1
#define TTF_DEBUG3 1
*/

const DBL TTF_Tolerance = 1.0e-6;    /* -4 worked, -8 failed */

const int MAX_ITERATIONS = 50;
const DBL COEFF_LIMIT = 1.0e-20;

/* For decoding glyph coordinate bit flags */
const int ONCURVE            = 0x01;
const int XSHORT             = 0x02;
const int YSHORT             = 0x04;
const int REPEAT_FLAGS       = 0x08;  /* repeat flag n times */
const int SHORT_X_IS_POS     = 0x10;  /* the short vector is positive */
const int NEXT_X_IS_ZERO     = 0x10;  /* the relative x coordinate is zero */
const int SHORT_Y_IS_POS     = 0x20;  /* the short vector is positive */
const int NEXT_Y_IS_ZERO     = 0x20;  /* the relative y coordinate is zero */

/* For decoding multi-component glyph bit flags */
const int ARG_1_AND_2_ARE_WORDS    = 0x0001;
const int ARGS_ARE_XY_VALUES       = 0x0002;
const int ROUND_XY_TO_GRID         = 0x0004;
const int WE_HAVE_A_SCALE          = 0x0008;
/*      RESERVED                 = 0x0010 */
const int MORE_COMPONENTS          = 0x0020;
const int WE_HAVE_AN_X_AND_Y_SCALE = 0x0040;
const int WE_HAVE_A_TWO_BY_TWO     = 0x0080;
const int WE_HAVE_INSTRUCTIONS     = 0x0100;
const int USE_MY_METRICS           = 0x0200;

/* For decoding kern coverage bit flags */
const int KERN_HORIZONTAL    = 0x01;
const int KERN_MINIMUM       = 0x02;
const int KERN_CROSS_STREAM  = 0x04;
const int KERN_OVERRIDE      = 0x08;

/* Some marcos to make error detection easier, as well as clarify code */
#define READSHORT(fp) readSHORT(fp, __LINE__, __FILE__)
#define READLONG(fp) readLONG(fp, __LINE__, __FILE__)
#define READUSHORT(fp) readUSHORT(fp, __LINE__, __FILE__)
#define READULONG(fp) readULONG(fp, __LINE__, __FILE__)
#define READFIXED(fp) readLONG(fp, __LINE__, __FILE__)
#define READFWORD(fp) readSHORT(fp, __LINE__, __FILE__)
#define READUFWORD(fp) readUSHORT(fp, __LINE__, __FILE__)

/*****************************************************************************
* Local typedefs
******************************************************************************/

/* Type definitions to match the TTF spec, makes code clearer */
typedef char CHAR;
typedef unsigned char BYTE;
typedef short SHORT;
typedef unsigned short USHORT;
typedef int LONG;
typedef unsigned int ULONG;
typedef short FWord;
typedef unsigned short uFWord;

typedef int Fixed;

typedef struct
{
  Fixed version;                /* 0x10000 (1.0) */
  USHORT numTables;             /* number of tables */
  USHORT searchRange;           /* (max2 <= numTables)*16 */
  USHORT entrySelector;         /* log2 (max2 <= numTables) */
  USHORT rangeShift;            /* numTables*16-searchRange */
} sfnt_OffsetTable;

typedef struct
{
  BYTE tag[4];
  ULONG checkSum;
  ULONG offset;
  ULONG length;
} sfnt_TableDirectory;

typedef sfnt_TableDirectory *sfnt_TableDirectoryPtr;

typedef struct
{
  ULONG bc;
  ULONG ad;
} longDateTime;

typedef struct
{
  Fixed version;                /* for this table, set to 1.0 */
  Fixed fontRevision;           /* For Font Manufacturer */
  ULONG checkSumAdjustment;
  ULONG magicNumber;            /* signature, must be 0x5F0F3CF5 == MAGIC */
  USHORT flags;
  USHORT unitsPerEm;            /* How many in Font Units per EM */

  longDateTime created;
  longDateTime modified;

  FWord xMin;                   /* Font wide bounding box in ideal space */
  FWord yMin;                   /* Baselines and metrics are NOT worked */
  FWord xMax;                   /* into these numbers) */
  FWord yMax;

  USHORT macStyle;              /* macintosh style word */
  USHORT lowestRecPPEM;         /* lowest recommended pixels per Em */

  SHORT fontDirectionHint;
  SHORT indexToLocFormat;       /* 0 - short offsets, 1 - long offsets */
  SHORT glyphDataFormat;
} sfnt_FontHeader;

typedef struct
{
  USHORT platformID;
  USHORT specificID;
  ULONG offset;
} sfnt_platformEntry;

typedef sfnt_platformEntry *sfnt_platformEntryPtr;

typedef struct
{
  USHORT format;
  USHORT length;
  USHORT version;
} sfnt_mappingTable;

typedef struct
{
  Fixed version;

  FWord Ascender;
  FWord Descender;
  FWord LineGap;

  uFWord advanceWidthMax;
  FWord minLeftSideBearing;
  FWord minRightSideBearing;
  FWord xMaxExtent;
  SHORT caretSlopeRise;
  SHORT caretSlopeRun;

  SHORT reserved1;
  SHORT reserved2;
  SHORT reserved3;
  SHORT reserved4;
  SHORT reserved5;

  SHORT metricDataFormat;
  USHORT numberOfHMetrics;      /* number of hMetrics in the hmtx table */
} sfnt_HorizHeader;

typedef struct
{
  SHORT numContours;
  SHORT xMin;
  SHORT yMin;
  SHORT xMax;
  SHORT yMax;
} GlyphHeader;

typedef struct
{
  GlyphHeader header;
  USHORT numPoints;
  USHORT *endPoints;
  BYTE *flags;
  DBL *x, *y;
  USHORT myMetrics;
} GlyphOutline;

typedef struct
{
  BYTE inside_flag;             /* 1 if this an inside contour, 0 if outside */
  USHORT count;                 /* Number of points in the contour */
  BYTE *flags;                  /* On/off curve flags */
  DBL *x, *y;                   /* Coordinates of control vertices */
} Contour;

typedef struct GlyphStruct *GlyphPtr;

/* Contour information for a single glyph */
typedef struct GlyphStruct
{
  GlyphHeader header;           /* Count and sizing information about this
                                 * glyph */
  USHORT glyph_index;           /* Internal glyph index for this character */
  Contour *contours;            /* Array of outline contours */
  USHORT unitsPerEm;            /* Max units character */
  GlyphPtr next;                /* Next cached glyph */
  USHORT c;                     /* Character code */
  USHORT myMetrics;             /* Which glyph index this is for metrics */
} Glyph;

typedef struct KernData_struct
{
  USHORT left, right;           /* Glyph index of left/right to kern */
  FWord value;                  /* Delta in FUnits to apply in between */
} KernData;

/*
 * [esp] There's already a "KernTable" on the Mac... renamed to TTKernTable for
 * now in memorium to its author.
 */

typedef struct KernStruct
{
  USHORT coverage;              /* Coverage bit field of this subtable */
  USHORT nPairs;                /* # of kerning pairs in this table */
  KernData *kern_pairs;         /* Array of kerning values */
} TTKernTable;

typedef struct KernTableStruct
{
  USHORT nTables;               /* # of subtables in the kerning table */
  TTKernTable *tables;
} KernTables;

typedef struct longHorMertric
{
  uFWord advanceWidth;          /* Total width of a glyph in FUnits */
  FWord lsb;                    /* FUnits to the left of the glyph */
} longHorMetric;

/* Useful general data about this font */
typedef struct FontFileInfoStruct
{
  char *filename;
  IStream *fp;
  USHORT platformID[4];             /* Character encoding search order */
  USHORT specificID[4];
  ULONG cmap_table_offset;          /* File locations for these tables */
  ULONG glyf_table_offset;
  USHORT numGlyphs;                 /* How many symbols in this file */
  USHORT unitsPerEm;                /* The "resoultion" of this font */
  SHORT indexToLocFormat;           /* 0 - short format, 1 - long format */
  ULONG *loca_table;                /* Mapping from characters to glyphs */
  GlyphPtr glyphs;                  /* Cached info for this font */
  KernTables kerning_tables;        /* Kerning info for this font */
  USHORT numberOfHMetrics;          /* The number of explicit spacings */
  longHorMetric *hmtx_table;        /* Horizontal spacing info */
  ULONG glyphIDoffset;              /* Offset for Type 4 encoding tables */
  USHORT segCount, searchRange,     /* Counts for Type 4 encoding tables */
         entrySelector, rangeShift;
  USHORT *startCount, *endCount,    /* Type 4 (MS) encoding tables */
         *idDelta, *idRangeOffset;
  struct FontFileInfoStruct *next;  /* Next font */
} FontFileInfo;

/*****************************************************************************
* Local variables
******************************************************************************/

const BYTE tag_CharToIndexMap[] = "cmap"; /* 0x636d6170; */
const BYTE tag_FontHeader[]     = "head"; /* 0x68656164; */
const BYTE tag_GlyphData[]      = "glyf"; /* 0x676c7966; */
const BYTE tag_IndexToLoc[]     = "loca"; /* 0x6c6f6361; */
const BYTE tag_Kerning[]        = "kern"; /* 0x6b65726e; */
const BYTE tag_MaxProfile[]     = "maxp"; /* 0x6d617870; */
const BYTE tag_HorizHeader[]    = "hhea"; /* 0x68686561; */
const BYTE tag_HorizMetric[]    = "hmtx"; /* 0x686d7478; */
const BYTE tag_TTCFontFile[]    = "ttcf"; /* */

static FontFileInfo *TTFonts = NULL; // GLOBAL VARIABLE

/*****************************************************************************
* Static functions
******************************************************************************/

/* Byte order independent I/O routines (probably already in other routines) */
static SHORT readSHORT (IStream *infile, int line, const char *file);
static USHORT readUSHORT (IStream *infile, int line, const char *file);
static LONG readLONG (IStream *infile, int line, const char *file);
static ULONG readULONG (IStream *infile, int line, const char *file);
static int compare_tag4 (BYTE *ttf_tag, BYTE *known_tag);

/* Internal TTF input routines */
static FontFileInfo *ProcessFontFile (char *fontfilename);
static FontFileInfo *OpenFontFile (char *filename);
static void ProcessHeadTable (FontFileInfo *ffile, int head_table_offset);
static void ProcessLocaTable (FontFileInfo *ffile, int loca_table_offset);
static void ProcessMaxpTable (FontFileInfo *ffile, int maxp_table_offset);
static void ProcessKernTable (FontFileInfo *ffile, int kern_table_offset);
static void ProcessHheaTable (FontFileInfo *ffile, int hhea_table_offset);
static void ProcessHmtxTable (FontFileInfo *ffile, int hmtx_table_offset);
static GlyphPtr ProcessCharacter (FontFileInfo *ffile, unsigned int search_char, unsigned int *glyph_index);
static USHORT ProcessCharMap (FontFileInfo *ffile, unsigned int search_char);
static USHORT ProcessFormat0Glyph (FontFileInfo *ffile, unsigned int search_char);
static USHORT ProcessFormat4Glyph (FontFileInfo *ffile, unsigned int search_char);
static USHORT ProcessFormat6Glyph (FontFileInfo *ffile, unsigned int search_char);
static GlyphPtr ExtractGlyphInfo (FontFileInfo *ffile, unsigned int *glyph_index, unsigned int c);
static GlyphOutline *ExtractGlyphOutline (FontFileInfo *ffile, unsigned int *glyph_index, unsigned int c);
static GlyphPtr ConvertOutlineToGlyph (FontFileInfo *ffile, GlyphOutline *ttglyph);


/* Glyph surface intersection routines */
static int Inside_Glyph (double x, double y, GlyphPtr glyph);
static int solve_quad (double *x, double *y, double mindist, DBL maxdist);
static void GetZeroOneHits (GlyphPtr glyph, VECTOR P, VECTOR D, DBL glyph_depth, double *t0, double *t1);
static int GlyphIntersect (OBJECT *Object, VECTOR P, VECTOR D, GlyphPtr glyph, DBL glyph_depth, RAY *Ray, ISTACK *Depth_Stack); /* tw */

/* POV-Ray object intersection/creation routines */
static TTF *Create_TTF (void);
static int All_TTF_Intersections (OBJECT *Object, RAY *Ray, ISTACK *Depth_Stack);
static int Inside_TTF (VECTOR IPoint, OBJECT *Object);
static void TTF_Normal (VECTOR Result, OBJECT *Object, INTERSECTION *Inter);
static TTF  *Copy_TTF (OBJECT *Object);
static void Translate_TTF (OBJECT *Object, VECTOR Vector, TRANSFORM *Trans);
static void Rotate_TTF (OBJECT *Object, VECTOR Vector, TRANSFORM *Trans);
static void Scale_TTF (OBJECT *Object, VECTOR Vector, TRANSFORM *Trans);
static void Transform_TTF (OBJECT *Object, TRANSFORM *Trans);
static void Invert_TTF (OBJECT *Object);
static void Destroy_TTF (OBJECT *Object);


METHODS TTF_Methods =
{All_TTF_Intersections,
 Inside_TTF, TTF_Normal, Default_UVCoord,
 (COPY_METHOD)Copy_TTF, Translate_TTF, Rotate_TTF, Scale_TTF, Transform_TTF,
 Invert_TTF, Destroy_TTF};

/*
 * The following work as macros if sizeof(short) == 16 bits and
 * sizeof(long) == 32 bits, but tend to break otherwise.  Making these
 * into error functions also allows file error checking.  Do not attempt to
 * "optimize" these functions - some architectures require them the way
 * that they are written.
 */
static SHORT readSHORT(IStream *infile, int line, const char *file)
{
  int i0, i1 = 0; /* To quiet warnings */

  if ((i0 = infile->Read_Byte ()) == EOF || (i1  = infile->Read_Byte ()) == EOF)
  {
    Error("Error reading TrueType font file at line %d, %s", line, file);
  }

  if (i0 & 0x80) /* Subtract 1 after value is negated to avoid overflow [AED] */
    return -(((255 - i0) << 8) | (255 - i1)) - 1;
  else
    return (i0 << 8) | i1;
}

static USHORT readUSHORT(IStream *infile, int line, const char *file)
{
  int i0, i1 = 0; /* To quiet warnings */

  if ((i0  = infile->Read_Byte ()) == EOF || (i1  = infile->Read_Byte ()) == EOF)
  {
    Error("Error reading TrueType font file at line %d, %s", line, file);
  }

  return (USHORT)((((USHORT)i0) << 8) | ((USHORT)i1));
}

static LONG readLONG(IStream *infile, int line, const char *file)
{
  LONG i0, i1 = 0, i2 = 0, i3 = 0; /* To quiet warnings */

   if ((i0 = infile->Read_Byte ()) == EOF || (i1 = infile->Read_Byte ()) == EOF ||
       (i2 = infile->Read_Byte ()) == EOF || (i3 = infile->Read_Byte ()) == EOF)
  {
    Error("Error reading TrueType font file at line %d, %s", line, file);
  }

  if (i0 & 0x80) /* Subtract 1 after value is negated to avoid overflow [AED] */
    return -(((255 - i0) << 24) | ((255 - i1) << 16) |
             ((255 - i2) << 8)  |  (255 - i3)) - 1;
  else
    return (i0 << 24) | (i1 << 16) | (i2 << 8) | i3;
}

static ULONG readULONG(IStream *infile, int line, const char *file)
{
  int i0, i1 = 0, i2 = 0, i3 = 0;  /* To quiet warnings */

  if ((i0 = infile->Read_Byte ()) == EOF || (i1 = infile->Read_Byte ()) == EOF ||
      (i2 = infile->Read_Byte ()) == EOF || (i3 = infile->Read_Byte ()) == EOF)
  {
    Error("Error reading TrueType font file at line %d, %s", line, file);
  }

  return (ULONG) ((((ULONG) i0) << 24) | (((ULONG) i1) << 16) |
                  (((ULONG) i2) << 8)  |  ((ULONG) i3));
}

static int compare_tag4(const BYTE *ttf_tag, const BYTE *known_tag)
{
   return (ttf_tag[0] == known_tag[0] && ttf_tag[1] == known_tag[1] &&
           ttf_tag[2] == known_tag[2] && ttf_tag[3] == known_tag[3]);
}

/*****************************************************************************
*
* FUNCTION
*
*   ProcessNewTTF
*
* INPUT
*   
* OUTPUT
*   
* RETURNS
*   
* AUTHOR
*
*   Alexander Ennzmann
*   
* DESCRIPTION
*
*   Takes an input string and a font filename, and creates a POV-Ray CSG
*   object for each letter in the string.
*
* CHANGES
*
*   -
*
******************************************************************************/
void ProcessNewTTF(OBJECT *object, char *filename, UCS2 *text_string, DBL depth, VECTOR offset)
{
    FontFileInfo *ffile;
    VECTOR local_offset, total_offset;
    CSG *Object = (CSG *) object;
    TTF *ttf;
    OBJECT *Local;
    GlyphPtr glyph;
    DBL funit_size;
    TTKernTable *table;
    USHORT coverage;
    unsigned int search_char;
    unsigned int glyph_index, last_index = 0;
    FWord kern_value_x, kern_value_min_x;
    FWord kern_value_y, kern_value_min_y;
    int i, j, k;
    TRANSFORM Trans;

    /* Get general font info */
    ffile = ProcessFontFile(filename);

    if((opts.Language_Version < 350) && (opts.String_Encoding == 0))
    {
        PossibleError("Text may not be displayed as expected.\n"
                      "Please refer to the user manual regarding changes\n"
                      "in POV-Ray 3.5 and later.");
    }

  /* Get info about each character in the string */
  Make_Vector(total_offset, 0.0, 0.0, 0.0);
  Object->Children = NULL;

  for (i = 0; text_string[i] != 0; i++)
  {
    /*
     * We need to make sure (for now) that this is only the lower 8 bits,
     * so we don't have all the high bits set if converted from a signed
     * char to an unsigned short.
     */
    search_char = (unsigned int)(text_string[i]);

#ifdef TTF_DEBUG
    Debug_Info("\nChar: '%c' (0x%X), Offset[%d]: <%g,%g,%g>\n", (char)search_char,
        search_char, i, total_offset[X], total_offset[Y], total_offset[Z]);
#endif

    /* Make a new child for each character */
    ttf = Create_TTF();
    Local = (OBJECT *) ttf;

    /* Set the depth information for the character */
    ttf->depth = depth;

    /*
     * Get pointers to the contour information for each character
     * in the text string.
     */
    glyph = ProcessCharacter(ffile, search_char, &glyph_index);
    ttf->glyph = (void *)glyph;
    funit_size = 1.0 / (DBL)(ffile->unitsPerEm);

    /*
     * Spacing based on the horizontal metric table, the kerning table,
     * and (possibly) the previous glyph.
     */
    if (i == 0) /* Ignore spacing on the left for the first character only */
    {
      /* Shift the glyph to start at the origin */
      total_offset[X] = -glyph->header.xMin * funit_size;

      Compute_Translation_Transform(&Trans, total_offset);

      Translate_TTF(Local, total_offset, &Trans);

      /* Shift next glyph by the width of this one excluding the left offset*/
      total_offset[X] = (ffile->hmtx_table[glyph->myMetrics].advanceWidth -
                         ffile->hmtx_table[glyph->myMetrics].lsb) * funit_size;

#ifdef TTF_DEBUG
      Debug_Info("aw(%d): %g\n", i,
                         (ffile->hmtx_table[glyph->myMetrics].advanceWidth -
                          ffile->hmtx_table[glyph->myMetrics].lsb)*funit_size);
#endif
    }
    else /* Kern all of the other characters */
    {
      kern_value_x = kern_value_y = 0;
      kern_value_min_x = kern_value_min_y = -ffile->unitsPerEm;
      Make_Vector(local_offset, 0.0, 0.0, 0.0);

      for (j = 0; j < ffile->kerning_tables.nTables; j++)
      {
        table = ffile->kerning_tables.tables;
        coverage = table->coverage;

        /*
         * Don't use vertical kerning until such a time when we support
         * characters moving in the vertical direction...
         */
        if (!(coverage & KERN_HORIZONTAL))
          continue;

        /*
         * If we were keen, we could do a binary search for this
         * character combination, since the pairs are sorted in 
         * order as if the left and right index values were a 32 bit 
         * unsigned int (mostly - at least they are sorted on the
         * left glyph).  Something to do when everything else works...
         */
        for (k = 0; k < table[j].nPairs; k++)
        {
          if (table[j].kern_pairs[k].left == last_index &&
              table[j].kern_pairs[k].right == glyph->myMetrics)
          {
#ifdef TTF_DEBUG2
            Debug_Info("Found a kerning for <%d, %d> = %d\n",
                       last_index, glyph_index, table[j].kern_pairs[k].value);
#endif

            /*
             * By default, Windows & OS/2 assume at most a single table with
             * !KERN_MINIMUM, !KERN_CROSS_STREAM, KERN_OVERRIDE.
             */
            if (coverage & KERN_MINIMUM)
            {
#ifdef TTF_DEBUG2
              Debug_Info(" KERN_MINIMUM\n");
#endif
              if (coverage & KERN_CROSS_STREAM)
                kern_value_min_y = table[j].kern_pairs[k].value;
              else
                kern_value_min_x = table[j].kern_pairs[k].value;
            }
            else
            {
              if (coverage & KERN_CROSS_STREAM)
              {
#ifdef TTF_DEBUG2
                Debug_Info(" KERN_CROSS_STREAM\n");
#endif
                if (table[j].kern_pairs[k].value == (FWord)0x8000)
                {
                  kern_value_y = 0;
                }
                else
                {
                  if (coverage & KERN_OVERRIDE)
                    kern_value_y = table[j].kern_pairs[k].value;
                  else 
                    kern_value_y += table[j].kern_pairs[k].value;
                }
              }
              else
              {
#ifdef TTF_DEBUG2
                Debug_Info(" KERN_VALUE\n");
#endif
                if (coverage & KERN_OVERRIDE)
                  kern_value_x = table[j].kern_pairs[k].value;
                else 
                  kern_value_x += table[j].kern_pairs[k].value;
              }
            }
            break;
          }
          /* Abort now if we have passed all potential matches */
          else if (table[j].kern_pairs[k].left > last_index)
          {
            break;
          }
        }
      }
      kern_value_x = (kern_value_x > kern_value_min_x ?
                      kern_value_x : kern_value_min_x);
      kern_value_y = (kern_value_y > kern_value_min_y ?
                      kern_value_y : kern_value_min_y);

      /*
       * Offset this character so that the left edge of the glyph is at
       * the previous offset + the lsb + any kerning amount.
       */
      local_offset[X] = total_offset[X] +
                        (DBL)(ffile->hmtx_table[glyph->myMetrics].lsb -
                              glyph->header.xMin + kern_value_x) * funit_size;
      local_offset[Y] = total_offset[Y] + (DBL)kern_value_y * funit_size;

      /* Translate this glyph to its final position in the string */
      Compute_Translation_Transform(&Trans, local_offset);

      Translate_TTF(Local, local_offset, &Trans);

      /* Shift next glyph by the width of this one + any kerning amount */
      total_offset[X] += (ffile->hmtx_table[glyph->myMetrics].advanceWidth +kern_value_x) * funit_size;

#ifdef TTF_DEBUG
      Debug_Info("kern(%d): <%d, %d> (%g,%g)\n", i, last_index, glyph_index,
                 (DBL)kern_value_x*funit_size, (DBL)kern_value_y * funit_size);
      Debug_Info("lsb(%d): %g\n", i,
                 (DBL)ffile->hmtx_table[glyph->myMetrics].lsb * funit_size);
      Debug_Info("aw(%d): %g\n", i,
                 (DBL)ffile->hmtx_table[glyph->myMetrics].advanceWidth *
                 funit_size);
#endif
    }

    /*
     * Add to the offset of the next character the minimum spacing specified.
     */
    VAddEq(total_offset, offset);

    /* Link this glyph with the others in the union */
    Object->Type |= (Local->Type & CHILDREN_FLAGS);
    Local->Type |= IS_CHILD_OBJECT;
    Local->Sibling = Object->Children;
    Object->Children = Local;

    last_index = glyph_index;
  }

#ifdef TTF_DEBUG
  Debug_Info("TTF parsing of \"%s\" from %s complete\n", text_string, filename);
#endif

  /* Close the font file descriptor */
  if(ffile->fp!=NULL)
  {
    delete ffile->fp;
    ffile->fp = NULL;
  }
}

/*****************************************************************************
*
* FUNCTION
*
*   ProcessFontFile
*
* INPUT
*
* OUTPUT
*   
* RETURNS
*   
* AUTHOR
*
*   Alexander Ennzmann
*   
* DESCRIPTION
*
* Read the header information about the specific font.  Parse the tables
* as we come across them.
*
* CHANGES
*
*   Added tests for reading manditory tables/validity checks - Jan 1996 [AED]
*   Reordered table parsing to avoid lots of file seeking - Jan 1996 [AED]
*
******************************************************************************/
static FontFileInfo *ProcessFontFile(char *fontfilename)
{
  unsigned i;
  int head_table_offset = 0;
  int loca_table_offset = 0;
  int maxp_table_offset = 0;
  int kern_table_offset = 0;
  int hhea_table_offset = 0;
  int hmtx_table_offset = 0;
  BYTE temp_tag[4];
  sfnt_OffsetTable OffsetTable;
  sfnt_TableDirectory Table;
  FontFileInfo *ffile;

  /* Open the font file */
  
  ffile = OpenFontFile(fontfilename);

  /* We have already read all the header info, no need to do it again */

  if (ffile->cmap_table_offset != 0)
  {
    return (ffile);
  }

  /*
   * Read the initial directory header on the TTF.  The numTables variable
   * tells us how many tables are present in this file.
   */
  if (!ffile->fp->read((char *)(&temp_tag), sizeof(BYTE) * 4))
  {
    Error("Cannot read TrueType font file table tag");
  }
  if (compare_tag4(temp_tag, tag_TTCFontFile))
  {
    READFIXED(ffile->fp); // header version - ignored [trf]
    READULONG(ffile->fp); // directory count - ignored [trf]
    // go to first font data block listed in the directory table entry [trf]
    ffile->fp->seekg(READULONG(ffile->fp), SEEK_SET);
  }
  else
  {
    // if it is no TTC style file, it is a regular TTF style file
    ffile->fp->seekg(0, SEEK_SET);
  }

  OffsetTable.version = READFIXED(ffile->fp);
  OffsetTable.numTables = READUSHORT(ffile->fp);
  OffsetTable.searchRange = READUSHORT(ffile->fp);
  OffsetTable.entrySelector = READUSHORT(ffile->fp);
  OffsetTable.rangeShift = READUSHORT(ffile->fp);

#ifdef TTF_DEBUG
  Debug_Info("OffsetTable:\n");
  Debug_Info("version=%d\n", OffsetTable.version);
  Debug_Info("numTables=%u\n", OffsetTable.numTables);
  Debug_Info("searchRange=%u\n", OffsetTable.searchRange);
  Debug_Info("entrySelector=%u\n", OffsetTable.entrySelector);
  Debug_Info("rangeShift=%u\n", OffsetTable.rangeShift);
#endif

  /*
   * I don't know why we limit this to 40 tables, since the spec says there
   * can be any number, but that's how it was when I got it.  Added a warning
   * just in case it ever happens in real life. [AED]
   */
  if (OffsetTable.numTables > 40)
  {
    Warning(0, "More than 40 (%d) TTF Tables in %s - some info may be lost!",
            OffsetTable.numTables, ffile->filename);
  }

  /* Process general font information and save it. */
  
  for (i = 0; i < OffsetTable.numTables && i < 40; i++)
  {
    if (!ffile->fp->read((char *)(&Table.tag), sizeof(BYTE) * 4))
    {
      Error("Cannot read TrueType font file table tag");
    }
    Table.checkSum = READULONG(ffile->fp);
    Table.offset   = READULONG(ffile->fp);
    Table.length   = READULONG(ffile->fp);

#ifdef TTF_DEBUG
    Debug_Info("\nTable %d:\n",i);
    Debug_Info("tag=%c%c%c%c\n", Table.tag[0], Table.tag[1],
                                 Table.tag[2], Table.tag[3]);
    Debug_Info("checkSum=%u\n", Table.checkSum);
    Debug_Info("offset=%u\n", Table.offset);
    Debug_Info("length=%u\n", Table.length);
#endif

    if (compare_tag4(Table.tag, tag_CharToIndexMap))
      ffile->cmap_table_offset = Table.offset;
    else if (compare_tag4(Table.tag, tag_GlyphData))
      ffile->glyf_table_offset = Table.offset;
    else if (compare_tag4(Table.tag, tag_FontHeader))
      head_table_offset = Table.offset;
    else if (compare_tag4(Table.tag, tag_IndexToLoc))
      loca_table_offset = Table.offset;
    else if (compare_tag4(Table.tag, tag_MaxProfile))
      maxp_table_offset = Table.offset;
    else if (compare_tag4(Table.tag, tag_Kerning))
      kern_table_offset = Table.offset;
    else if (compare_tag4(Table.tag, tag_HorizHeader))
      hhea_table_offset = Table.offset;
    else if (compare_tag4(Table.tag, tag_HorizMetric))
      hmtx_table_offset = Table.offset;
  }

  if (ffile->cmap_table_offset == 0 || ffile->glyf_table_offset == 0 ||
      head_table_offset == 0 || loca_table_offset == 0 ||
      hhea_table_offset == 0 || hmtx_table_offset == 0 ||
      maxp_table_offset == 0)
  {
    Error("Invalid TrueType font headers in %s", ffile->filename);
  }

  ProcessHeadTable(ffile, head_table_offset);  /* Need indexToLocFormat */
  if ((ffile->indexToLocFormat != 0 && ffile->indexToLocFormat != 1) ||
      (ffile->unitsPerEm < 16 || ffile->unitsPerEm > 16384))
    Error("Invalid TrueType font data in %s", ffile->filename);

  ProcessMaxpTable(ffile, maxp_table_offset);  /* Need numGlyphs */
  if (ffile->numGlyphs <= 0)
    Error("Invalid TrueType font data in %s", ffile->filename);

  ProcessLocaTable(ffile, loca_table_offset);  /* Now we can do loca_table */

  ProcessHheaTable(ffile, hhea_table_offset);  /* Need numberOfHMetrics */
  if (ffile->numberOfHMetrics <= 0)
    Error("Invalid TrueType font data in %s", ffile->filename);

  ProcessHmtxTable(ffile, hmtx_table_offset);  /* Now we can read HMetrics */

  if (kern_table_offset != 0)
      ProcessKernTable(ffile, kern_table_offset);

  /* Return the information about this font */
  
  return ffile;
}

/*****************************************************************************
*
* FUNCTION
*
*   OpenFontFile
*
* INPUT
*   
* OUTPUT
*   
* RETURNS
*   
* AUTHOR
*
*   Alexander Ennzmann
*   
* DESCRIPTION
*
*   -
*
* CHANGES
*
*   -
*
******************************************************************************/
static FontFileInfo *OpenFontFile(char *filename)
{
  /* int i; */ /* tw, mtg */
  FontFileInfo *fontlist;
  char b[FILE_NAME_LENGTH];

  /* First look to see if we have already opened this font */
  
  for(fontlist = TTFonts; fontlist != NULL; fontlist = fontlist->next)
    if(!strcmp(filename, fontlist->filename))
      break;

  if(fontlist != NULL)
  {
    if(fontlist->fp == NULL)
    {
      /* We have a match, use the previous information */
      fontlist->fp = Locate_File(fontlist->filename,POV_File_Font_TTF,NULL,true);
      if(fontlist->fp == NULL)
      {
        Error("Cannot open font file.");
      }
    }
    else
    {
      #ifdef TTF_DEBUG
      Debug_Info("Using cached font info for %s\n", fontlist->filename);
      #endif
    }
  }
  else
  {
    /*
     * We haven't looked at this font before, let's allocate a holder for the
     * information and set some defaults
     */

    fontlist = (FontFileInfo *)POV_CALLOC(1, sizeof(FontFileInfo), "FontFileInfo");
    
    if((fontlist->fp = Locate_File(filename,POV_File_Font_TTF,b,true)) == NULL)
    {
      Error("Cannot open font file.");
    }
    
    fontlist->filename = POV_STRDUP(b);
    
    /*
     * For Microsoft encodings 3, 1 is for Unicode
     *                         3, 0 is for Non-Unicode (ie symbols)
     * For Macintosh encodings 1, 0 is for Roman character set
     * For Unicode encodings   0, 3 is for Unicode
     */
    switch(opts.String_Encoding)
    {
        case 0: // ASCII
            // first choice
            fontlist->platformID[0] = 1;
            fontlist->specificID[0] = 0;
            // second choice
            fontlist->platformID[1] = 3;
            fontlist->specificID[1] = 1;
            // third choice
            fontlist->platformID[2] = 0;
            fontlist->specificID[2] = 3;
            // fourth choice
            fontlist->platformID[3] = 3;
            fontlist->specificID[3] = 0;
            break;
        case 1: // UTF8
        case 2: // System Specific
            // first choice
            fontlist->platformID[0] = 0;
            fontlist->specificID[0] = 3;
            // second choice
            fontlist->platformID[1] = 3;
            fontlist->specificID[1] = 1;
            // third choice
            fontlist->platformID[2] = 1;
            fontlist->specificID[2] = 0;
            // fourth choice
            fontlist->platformID[3] = 3;
            fontlist->specificID[3] = 0;
            break;
    }
    fontlist->next = TTFonts;
    TTFonts = fontlist;
  }

  return fontlist;
}

void FreeFontInfo()
{
  int i;
  FontFileInfo *oldfont, *tempfont;
  GlyphPtr glyphs, tempglyph;

  for (oldfont = TTFonts; oldfont != NULL;)
  {
    if (oldfont->fp != NULL)
      delete oldfont->fp;
    
    if (oldfont->filename != NULL)
      POV_FREE(oldfont->filename);
    
    if (oldfont->loca_table != NULL)
      POV_FREE(oldfont->loca_table);
    
    if (oldfont->hmtx_table != NULL)
      POV_FREE(oldfont->hmtx_table);

    if (oldfont->kerning_tables.nTables != 0)
    {
      for (i = 0; i < oldfont->kerning_tables.nTables; i++)
      {
        if (oldfont->kerning_tables.tables[i].kern_pairs)
          POV_FREE(oldfont->kerning_tables.tables[i].kern_pairs);
      }
    
      POV_FREE(oldfont->kerning_tables.tables);
    }
    
    for (glyphs = oldfont->glyphs; glyphs != NULL;)
    {
      for (i = 0; i < glyphs->header.numContours; i++)
      {
        POV_FREE(glyphs->contours[i].flags);
        POV_FREE(glyphs->contours[i].x);
        POV_FREE(glyphs->contours[i].y);
      }
  
      if (glyphs->contours != NULL)
        POV_FREE(glyphs->contours);

      tempglyph = glyphs;
      glyphs = glyphs->next;
      POV_FREE(tempglyph);
    }
    
    if (oldfont->segCount != 0)
    {
      POV_FREE(oldfont->endCount);
      POV_FREE(oldfont->startCount);
      POV_FREE(oldfont->idDelta);
      POV_FREE(oldfont->idRangeOffset);
    }

    tempfont = oldfont;
    oldfont = oldfont->next;
    POV_FREE(tempfont);
  }

  TTFonts = NULL;
}

/* Process the font header table */
static void ProcessHeadTable(FontFileInfo *ffile, int head_table_offset)
{
  sfnt_FontHeader fontHeader;

  /* Read head table */
  ffile->fp->seekg(head_table_offset) ;

  fontHeader.version = READFIXED(ffile->fp);
  fontHeader.fontRevision = READFIXED(ffile->fp);
  fontHeader.checkSumAdjustment = READULONG(ffile->fp);
  fontHeader.magicNumber = READULONG(ffile->fp);   /* should be 0x5F0F3CF5 */
  fontHeader.flags = READUSHORT(ffile->fp);
  fontHeader.unitsPerEm = READUSHORT(ffile->fp);
  fontHeader.created.bc = READULONG(ffile->fp);
  fontHeader.created.ad = READULONG(ffile->fp);
  fontHeader.modified.bc = READULONG(ffile->fp);
  fontHeader.modified.ad = READULONG(ffile->fp);
  fontHeader.xMin = READFWORD(ffile->fp);
  fontHeader.yMin = READFWORD(ffile->fp);
  fontHeader.xMax = READFWORD(ffile->fp);
  fontHeader.yMax = READFWORD(ffile->fp);
  fontHeader.macStyle = READUSHORT(ffile->fp);
  fontHeader.lowestRecPPEM = READUSHORT(ffile->fp);
  fontHeader.fontDirectionHint = READSHORT(ffile->fp);
  fontHeader.indexToLocFormat = READSHORT(ffile->fp);
  fontHeader.glyphDataFormat = READSHORT(ffile->fp);

#ifdef TTF_DEBUG
  Debug_Info("\nfontHeader:\n");
  Debug_Info("version: %d\n",fontHeader.version);
  Debug_Info("fontRevision: %d\n",fontHeader.fontRevision);
  Debug_Info("checkSumAdjustment: %u\n",fontHeader.checkSumAdjustment);
  Debug_Info("magicNumber: 0x%8X\n",fontHeader.magicNumber);
  Debug_Info("flags: %u\n",fontHeader.flags);
  Debug_Info("unitsPerEm: %u\n",fontHeader.unitsPerEm);
  Debug_Info("created.bc: %u\n",fontHeader.created.bc);
  Debug_Info("created.ad: %u\n",fontHeader.created.ad);
  Debug_Info("modified.bc: %u\n",fontHeader.modified.bc);
  Debug_Info("modified.ad: %u\n",fontHeader.modified.ad);
  Debug_Info("xMin: %d\n",fontHeader.xMin);
  Debug_Info("yMin: %d\n",fontHeader.yMin);
  Debug_Info("xMax: %d\n",fontHeader.xMax);
  Debug_Info("yMax: %d\n",fontHeader.yMax);
  Debug_Info("macStyle: %u\n",fontHeader.macStyle);
  Debug_Info("lowestRecPPEM: %u\n",fontHeader.lowestRecPPEM);
  Debug_Info("fontDirectionHint: %d\n",fontHeader.fontDirectionHint);
  Debug_Info("indexToLocFormat: %d\n",fontHeader.indexToLocFormat);
  Debug_Info("glyphDataFormat: %d\n",fontHeader.glyphDataFormat);
#endif
  
  if (fontHeader.magicNumber != 0x5F0F3CF5)
  {
    Error("Error reading TrueType font %s.  Bad magic number (0x%8X)",
          ffile->filename, fontHeader.magicNumber);
  }

  ffile->indexToLocFormat = fontHeader.indexToLocFormat;
  ffile->unitsPerEm = fontHeader.unitsPerEm;
}

/* Determine the relative offsets of glyphs */
static void ProcessLocaTable(FontFileInfo *ffile, int loca_table_offset)
{
  int i;

  /* Move to location of table in file */
  ffile->fp->seekg(loca_table_offset) ;

  ffile->loca_table = (ULONG *)POV_MALLOC((ffile->numGlyphs+1) * sizeof(ULONG), "ttf");

#ifdef TTF_DEBUG
  Debug_Info("\nlocation table:\n");
  Debug_Info("version: %s\n",(ffile->indexToLocFormat?"long":"short"));
#endif

  /* Now read and save the location table */
  
  if (ffile->indexToLocFormat == 0)                  /* short version */
  {
    for (i = 0; i < ffile->numGlyphs; i++)
    {
      ffile->loca_table[i] = ((ULONG)READUSHORT(ffile->fp)) << 1;
#ifdef TTF_DEBUG2
      Debug_Info("loca_table[%d] @ %u\n", i, ffile->loca_table[i]);
#endif
    }
  }
  else                                               /* long version */
  {
    for (i = 0; i < ffile->numGlyphs; i++)
    {
      ffile->loca_table[i] = READULONG(ffile->fp);
#ifdef TTF_DEBUG2
      Debug_Info("loca_table[%d] @ %u\n", i, ffile->loca_table[i]);
#endif
    }
  }
}


/*
 * This routine determines the total number of glyphs in a TrueType file.
 * Necessary so that we can allocate the proper amount of storage for the glyph
 * location table.
 */
static void ProcessMaxpTable(FontFileInfo *ffile, int maxp_table_offset)
{
  /* seekg to the maxp table, skipping the 4 byte version number */
  ffile->fp->seekg(maxp_table_offset + 4) ;
  
  ffile->numGlyphs = READUSHORT(ffile->fp);

#ifdef TTF_DEBUG
  Debug_Info("\nmaximum profile table:\n");
  Debug_Info("numGlyphs: %u\n", ffile->numGlyphs);
#endif
}


/* Read the kerning information for a glyph */
static void ProcessKernTable(FontFileInfo *ffile, int kern_table_offset)
{
  int i, j;
  USHORT temp16;
  USHORT length;
  KernTables *kern_table;

  kern_table = &ffile->kerning_tables;

  /* Move to the beginning of the kerning table, skipping the 2 byte version */
  ffile->fp->seekg(kern_table_offset + 2) ;

  /* Read in the number of kerning tables */

  kern_table->nTables = READUSHORT(ffile->fp);
  kern_table->tables = NULL;      /*<==[esp] added (in case nTables is zero)*/
  
#ifdef TTF_DEBUG
  Debug_Info("\nKerning table:\n", kern_table_offset);
  Debug_Info("Offset: %d\n", kern_table_offset);
  Debug_Info("Number of tables: %u\n",kern_table->nTables);
#endif

  /* Don't do any more work if there isn't kerning info */
  
  if (kern_table->nTables == 0)
    return;

  kern_table->tables = (TTKernTable *)POV_MALLOC(kern_table->nTables * sizeof(TTKernTable),
                                  "ProcessKernTable");
  
  for (i = 0; i < kern_table->nTables; i++)
  {
    /* Read in a subtable */
    
    temp16 = READUSHORT(ffile->fp);                      /* Subtable version */
    length = READUSHORT(ffile->fp);                       /* Subtable length */
    kern_table->tables[i].coverage = READUSHORT(ffile->fp); /* Coverage bits */
    
#ifdef TTF_DEBUG
    Debug_Info("Coverage table[%d] (0x%X):", i, kern_table->tables[i].coverage);
    Debug_Info("  type %u", (kern_table->tables[i].coverage >> 8));
    Debug_Info(" %s", (kern_table->tables[i].coverage & KERN_HORIZONTAL ?
                         "Horizontal" : "Vertical" ));
    Debug_Info(" %s values", (kern_table->tables[i].coverage & KERN_MINIMUM ?
                         "Minimum" : "Kerning" ));
    Debug_Info("%s", (kern_table->tables[i].coverage & KERN_CROSS_STREAM ?
                         " Cross-stream" : "" ));
    Debug_Info("%s\n", (kern_table->tables[i].coverage & KERN_OVERRIDE ?
                         " Override" : "" ));
#endif

    kern_table->tables[i].kern_pairs = NULL;   /*<==[esp] added*/
    kern_table->tables[i].nPairs = 0;          /*<==[esp] added*/

    if ((kern_table->tables[i].coverage >> 8) == 0)
    {
      /* Can only handle format 0 kerning subtables */
      kern_table->tables[i].nPairs = READUSHORT(ffile->fp);

#ifdef TTF_DEBUG
      Debug_Info("entries in table[%d]: %d\n", i, kern_table->tables[i].nPairs);
#endif

      temp16 = READUSHORT(ffile->fp);     /* searchRange */
      temp16 = READUSHORT(ffile->fp);     /* entrySelector */
      temp16 = READUSHORT(ffile->fp);     /* rangeShift */
      
      kern_table->tables[i].kern_pairs =
      (KernData *)POV_MALLOC(kern_table->tables[i].nPairs * sizeof(KernData), "Kern Pairs");

      for (j = 0; j < kern_table->tables[i].nPairs; j++)
      {
        /* Read in a kerning pair */
        kern_table->tables[i].kern_pairs[j].left = READUSHORT(ffile->fp);
        kern_table->tables[i].kern_pairs[j].right = READUSHORT(ffile->fp);
        kern_table->tables[i].kern_pairs[j].value = READFWORD(ffile->fp);

#ifdef TTF_DEBUG2
        Debug_Info("Kern pair: <%d,%d> = %d\n",
                   (int)kern_table->tables[i].kern_pairs[j].left,
                   (int)kern_table->tables[i].kern_pairs[j].right,
                   (int)kern_table->tables[i].kern_pairs[j].value);
#endif
      }
    }
    else
    {
#ifdef TTF_DEBUG2
      Warning(0, "Cannot handle format %u kerning data",
              (kern_table->tables[i].coverage >> 8));
#endif
      /*
       * seekg to the end of this table, excluding the length of the version,
       * length, and coverage USHORTs, which we have already read.
       */
      ffile->fp->seekg((int)(length - 6), POV_SEEK_CUR) ;
      kern_table->tables[i].nPairs = 0;
    }
  }
}

/*
 * This routine determines the total number of horizontal metrics.
 */
static void ProcessHheaTable(FontFileInfo *ffile, int hhea_table_offset)
{
#ifdef TTF_DEBUG
  sfnt_HorizHeader horizHeader;

  /* seekg to the hhea table */
  ffile->fp->seekg(hhea_table_offset);
  
  horizHeader.version = READFIXED(ffile->fp);
  horizHeader.Ascender = READFWORD(ffile->fp);
  horizHeader.Descender = READFWORD(ffile->fp);
  horizHeader.LineGap = READFWORD(ffile->fp);
  horizHeader.advanceWidthMax = READUFWORD(ffile->fp);
  horizHeader.minLeftSideBearing = READFWORD(ffile->fp);
  horizHeader.minRightSideBearing = READFWORD(ffile->fp);
  horizHeader.xMaxExtent = READFWORD(ffile->fp);
  horizHeader.caretSlopeRise = READSHORT(ffile->fp);
  horizHeader.caretSlopeRun = READSHORT(ffile->fp);
  horizHeader.reserved1 = READSHORT(ffile->fp);
  horizHeader.reserved2 = READSHORT(ffile->fp);
  horizHeader.reserved3 = READSHORT(ffile->fp);
  horizHeader.reserved4 = READSHORT(ffile->fp);
  horizHeader.reserved5 = READSHORT(ffile->fp);
  horizHeader.metricDataFormat = READSHORT(ffile->fp);
#else

  /* seekg to the hhea table, skipping all that stuff we don't need */
  ffile->fp->seekg (hhea_table_offset + 34) ;
  
#endif

  ffile->numberOfHMetrics = READUSHORT(ffile->fp);

#ifdef TTF_DEBUG
  Debug_Info("\nhorizontal header table:\n");
  Debug_Info("Ascender: %d\n",horizHeader.Ascender);
  Debug_Info("Descender: %d\n",horizHeader.Descender);
  Debug_Info("LineGap: %d\n",horizHeader.LineGap);
  Debug_Info("advanceWidthMax: %d\n",horizHeader.advanceWidthMax);
  Debug_Info("minLeftSideBearing: %d\n",horizHeader.minLeftSideBearing);
  Debug_Info("minRightSideBearing: %d\n",horizHeader.minRightSideBearing);
  Debug_Info("xMaxExtent: %d\n",horizHeader.xMaxExtent);
  Debug_Info("caretSlopeRise: %d\n",horizHeader.caretSlopeRise);
  Debug_Info("caretSlopeRun: %d\n",horizHeader.caretSlopeRun);
  Debug_Info("metricDataFormat: %d\n",horizHeader.metricDataFormat);
  Debug_Info("numberOfHMetrics: %d\n",ffile->numberOfHMetrics);
#endif
}

static void ProcessHmtxTable (FontFileInfo *ffile, int hmtx_table_offset)
{
  int i;
  longHorMetric *metric;
  uFWord lastAW = 0;     /* Just to quiet warnings. */

  ffile->fp->seekg (hmtx_table_offset) ;

  ffile->hmtx_table = (longHorMetric *)POV_MALLOC(ffile->numGlyphs*sizeof(longHorMetric), "ttf");

  /*
   * Read in the total glyph width, and the left side offset.  There is
   * guaranteed to be at least one longHorMetric entry in this table to
   * set the advanceWidth for the subsequent lsb entries.
   */
  for (i=0, metric=ffile->hmtx_table; i < ffile->numberOfHMetrics; i++,metric++)
  {
    lastAW = metric->advanceWidth = READUFWORD(ffile->fp);
    metric->lsb = READFWORD(ffile->fp);
  }

  /* Read in the remaining left offsets */
  for (; i < ffile->numGlyphs; i++, metric++)
  {
    metric->advanceWidth = lastAW;
    metric->lsb = READFWORD(ffile->fp);
  }
}

/*****************************************************************************
*
* FUNCTION
*
*   ProcessCharacter
*
* INPUT
*   
* OUTPUT
*   
* RETURNS
*   
* AUTHOR
*
*   POV-Ray Team
*   
* DESCRIPTION
*
*   Finds the glyph description for the current character.
*
* CHANGES
*
*   -
*
******************************************************************************/

static GlyphPtr ProcessCharacter(FontFileInfo *ffile, unsigned int search_char, unsigned int *glyph_index)
{
  GlyphPtr glyph;

  /* See if we have already processed this glyph */
  for (glyph = ffile->glyphs; glyph != NULL; glyph = glyph->next)
  {
    if (glyph->c == search_char)
    {
      /* Found it, no need to do any more work */
#ifdef TTF_DEBUG
      Debug_Info("Cached glyph: %c/%u\n",(char)search_char,glyph->glyph_index);
#endif
      *glyph_index = glyph->glyph_index;
      return glyph;
    }
  }

  *glyph_index = ProcessCharMap(ffile, search_char);

  if (*glyph_index == 0)
    Warning(0, "Character %d (0x%X) not found in %s", (BYTE)search_char,
            search_char, ffile->filename);

  /* See if we have already processed this glyph (using the glyph index) */
  for (glyph = ffile->glyphs; glyph != NULL; glyph = glyph->next)
  {
    if (glyph->glyph_index == *glyph_index)
    {
      /* Found it, no need to do any more work */
#ifdef TTF_DEBUG
      Debug_Info("Cached glyph: %c/%u\n",(char)search_char,glyph->glyph_index);
#endif
      *glyph_index = glyph->glyph_index;
      return glyph;
    }
  }

  glyph = ExtractGlyphInfo(ffile, glyph_index, search_char);

  /* Add this glyph to the ones we already know about */
  
  glyph->next = ffile->glyphs;
  ffile->glyphs = glyph;

  /* Glyph is all built */
  
  return glyph;
}

/*****************************************************************************
*
* FUNCTION
*
*   ProcessCharMap
*
* INPUT
*   
* OUTPUT
*   
* RETURNS
*   
* AUTHOR
*
*   POV-Ray Team
*   
* DESCRIPTION
*
*   Find the character mapping for 'search_char'.  We should really know
*   which character set we are using (ie ISO 8859-1, Mac, Unicode, etc).
*   Search char should really be a USHORT to handle double byte systems.
*
* CHANGES
*
*   961120  esp  Added check to allow Macintosh encodings to pass
*
******************************************************************************/
static USHORT ProcessCharMap(FontFileInfo *ffile, unsigned int search_char)
{
    int initial_table_offset;
    int old_table_offset;
    int entry_offset;
    sfnt_platformEntry cmapEntry;
    sfnt_mappingTable encodingTable;
    int i, j, table_count;

    /* Move to the start of the character map, skipping the 2 byte version */
    ffile->fp->seekg (ffile->cmap_table_offset + 2) ;

    table_count = READUSHORT(ffile->fp);

    #ifdef TTF_DEBUG
    Debug_Info("table_count=%d\n", table_count);
    #endif

    /*
     * Search the tables until we find the glyph index for the search character.
     * Just return the first one we find...
     */

    initial_table_offset = ffile->fp->tellg (); /* Save the initial position */

    for(j = 0; j <= 3; j++)
    {
        ffile->fp->seekg(initial_table_offset); /* Always start new search at the initial position */

        for (i = 0; i < table_count; i++)
        {
            cmapEntry.platformID = READUSHORT(ffile->fp);
            cmapEntry.specificID = READUSHORT(ffile->fp);
            cmapEntry.offset     = READULONG(ffile->fp);

            #ifdef TTF_DEBUG
            Debug_Info("cmapEntry: platformID=%d\n", cmapEntry.platformID);
            Debug_Info("cmapEntry: specificID=%d\n", cmapEntry.specificID);
            Debug_Info("cmapEntry: offset=%d\n", cmapEntry.offset);
            #endif

            /*
             * Check if this is the encoding table we want to use.
             * The search is done according to user preference.
             */
            if ( ffile->platformID[j] != cmapEntry.platformID ) /* [JAC 01/99] */
            {
                continue;
            }

            entry_offset = cmapEntry.offset;

            old_table_offset = ffile->fp->tellg (); /* Save the current position */

            ffile->fp->seekg (ffile->cmap_table_offset + entry_offset) ;

            encodingTable.format = READUSHORT(ffile->fp);
            encodingTable.length = READUSHORT(ffile->fp);
            encodingTable.version = READUSHORT(ffile->fp);

            #ifdef TTF_DEBUG
            Debug_Info("Encoding table, format: %u, length: %u, version: %u\n",
                       encodingTable.format, encodingTable.length, encodingTable.version);
            #endif

            if (encodingTable.format == 0)
            {
                /*
                 * Translation is simple - add 'entry_char' to the start of the
                 * table and grab what's there.
                 */
                #ifdef TTF_DEBUG
                Debug_Info("Apple standard index mapping\n");
                #endif

                return(ProcessFormat0Glyph(ffile, search_char));
            }
            #if 0  /* Want to get the rest of these working first */
            else if (encodingTable.format == 2)
            {
                /* Used for multi-byte character encoding (Chinese, Japanese, etc) */
                #ifdef TTF_DEBUG
                Debug_Info("High-byte index mapping\n");
                #endif

                return(ProcessFormat2Glyph(ffile, search_char));
            }
            #endif
            else if (encodingTable.format == 4)
            {
                /* Microsoft UGL encoding */
                #ifdef TTF_DEBUG
                Debug_Info("Microsoft standard index mapping\n");
                #endif

                return(ProcessFormat4Glyph(ffile, search_char));
            }
            else if (encodingTable.format == 6)
            {
                #ifdef TTF_DEBUG
                Debug_Info("Trimmed table mapping\n");
                #endif

                return(ProcessFormat6Glyph(ffile, search_char));
            }
            #ifdef TTF_DEBUG
            else
                Debug_Info("Unsupported TrueType font index mapping format: %u\n",
                           encodingTable.format);
            #endif

            /* Go to the next table entry if we didn't find a match */
            ffile->fp->seekg (old_table_offset) ;
        }
    }

    /*
     * No character mapping was found - very odd, we should really have had the
     * character in at least one table.  Perhaps getting here means we didn't
     * have any character mapping tables.  '0' means no mapping.
     */
    
    return 0;
}


/*****************************************************************************
*
* FUNCTION
*
*   ProcessFormat0Glyph
*
* INPUT
*   
* OUTPUT
*   
* RETURNS
*   
* AUTHOR
*
*   POV-Ray Team
*   
* DESCRIPTION
*
* This handles the Apple standard index mapping for glyphs.
* The file pointer must be pointing immediately after the version entry in the
* encoding table for the next two functions to work.
*
* CHANGES
*
*   -
*
******************************************************************************/
static USHORT ProcessFormat0Glyph(FontFileInfo *ffile, unsigned int search_char)
{
  BYTE temp_index;

  ffile->fp->seekg ((int)search_char, POV_SEEK_CUR) ;
  
  if (!ffile->fp->read ((char *)(&temp_index), 1)) /* Each index is 1 byte */
  {
    Error("Cannot read TrueType font file at line %d, %s",
          __LINE__, __FILE__);
  }
  
  return (USHORT)(temp_index);
}

/*****************************************************************************
*
* FUNCTION
*
*   ProcessFormat4Glyph
*
* INPUT
*   
* OUTPUT
*   
* RETURNS
*   
* AUTHOR
*
*   POV-Ray Team
*   
* DESCRIPTION
*
* This handles the Microsoft standard index mapping for glyph tables
*
* CHANGES
*
*   Mar 26, 1996: Cache segment info rather than read each time.  [AED]
*
******************************************************************************/
static USHORT ProcessFormat4Glyph(FontFileInfo *ffile, unsigned int search_char)
{
  int i;
  unsigned int glyph_index = 0;  /* Set the glyph index to "not present" */

  /*
   * If this is the first time we are here, read all of the segment headers,
   * and save them for later calls to this function, rather than seeking and
   * mallocing for each character
   */
  if (ffile->segCount == 0)
  {
    USHORT temp16;

    ffile->segCount = READUSHORT(ffile->fp) >> 1;
    ffile->searchRange = READUSHORT(ffile->fp);
    ffile->entrySelector = READUSHORT(ffile->fp);
    ffile->rangeShift = READUSHORT(ffile->fp);

    /* Now allocate and read in the segment arrays */
  
    ffile->endCount = (USHORT *)POV_MALLOC(ffile->segCount * sizeof(USHORT), "ttf");
    ffile->startCount = (USHORT *)POV_MALLOC(ffile->segCount * sizeof(USHORT), "ttf");
    ffile->idDelta = (USHORT *)POV_MALLOC(ffile->segCount * sizeof(USHORT), "ttf");
    ffile->idRangeOffset = (USHORT *)POV_MALLOC(ffile->segCount * sizeof(USHORT), "ttf");

    for (i = 0; i < ffile->segCount; i++)
    {
      ffile->endCount[i] = READUSHORT(ffile->fp);
    }
  
    temp16 = READUSHORT(ffile->fp);  /* Skip over 'reservedPad' */
  
    for (i = 0; i < ffile->segCount; i++)
    {
      ffile->startCount[i] = READUSHORT(ffile->fp);
    }
  
    for (i = 0; i < ffile->segCount; i++)
    {
      ffile->idDelta[i] = READUSHORT(ffile->fp);
    }

    /* location of start of idRangeOffset */
    ffile->glyphIDoffset = ffile->fp->tellg () ;

    for (i = 0; i < ffile->segCount; i++)
    {
      ffile->idRangeOffset[i] = READUSHORT(ffile->fp);
    }
  }
  
  /* Search the segments for our character */
  
glyph_search:
  for (i = 0; i < ffile->segCount; i++)
  {
    if (search_char <= ffile->endCount[i])
    {
      if (search_char >= ffile->startCount[i])
      {
        /* Found correct range for this character */
  
        if (ffile->idRangeOffset[i] == 0)
        {
          glyph_index = search_char + ffile->idDelta[i];
        }
        else
        {
          /*
           * Alternate encoding of glyph indices, relies on a quite unusual way
           * of storing the offsets.  We need the *2s because we are talking
           * about addresses of shorts and not bytes.
           *
           * (glyphIDoffset + i*2 + idRangeOffset[i]) == &idRangeOffset[i]
           */
          ffile->fp->seekg (ffile->glyphIDoffset + 2*i + ffile->idRangeOffset[i]+
                           2*(search_char - ffile->startCount[i]));
          
          glyph_index = READUSHORT(ffile->fp);
          
          if (glyph_index != 0)
            glyph_index = glyph_index + ffile->idDelta[i];
        }
      }
      break;
    }
  }

  /*
   * If we haven't found the character yet, and this is the first time to
   * search the tables, try looking in the Unicode user space, since this
   * is the location Microsoft recommends for symbol characters like those
   * in wingdings and dingbats.
   */
  if (glyph_index == 0 && search_char < 0x100)
  {
    search_char += 0xF000;
#ifdef TTF_DEBUG
    Debug_Info("Looking for glyph in Unicode user space (0x%X)\n", search_char);
#endif
    goto glyph_search;
  }

  /* Deallocate the memory we used for the segment arrays */
  
  return glyph_index;
}

/*****************************************************************************
*
* FUNCTION
*
*   ProcessFormat6Glyph
*
* INPUT
*   
* OUTPUT
*   
* RETURNS
*   
* AUTHOR
*
*   POV-Ray Team
*   
* DESCRIPTION
*
*  This handles the trimmed table mapping for glyphs.
*
* CHANGES
*
*   -
*
******************************************************************************/
static USHORT ProcessFormat6Glyph(FontFileInfo *ffile, unsigned int search_char)
{
  USHORT firstCode, entryCount;
  BYTE glyph_index;

  firstCode = READUSHORT(ffile->fp);
  entryCount = READUSHORT(ffile->fp);
  
  if (search_char >= firstCode && search_char < firstCode + entryCount)
  {
    ffile->fp->seekg ((int)(search_char - firstCode), POV_SEEK_CUR) ;
    glyph_index = READUSHORT(ffile->fp);
  }
  else
    glyph_index = 0;
  
  return glyph_index;
}


/*****************************************************************************
*
* FUNCTION
*
*   ExtractGlyphInfo
*
* INPUT
*   
* OUTPUT
*   
* RETURNS
*   
* AUTHOR
*
*   POV-Ray Team
*   
* DESCRIPTION
*
*   Change TTF outline information for the glyph(s) into a useful format
*
* CHANGES
*
*   -
*
******************************************************************************/
static GlyphPtr ExtractGlyphInfo(FontFileInfo *ffile, unsigned int *glyph_index, unsigned int c)
{
  GlyphOutline *ttglyph;
  GlyphPtr glyph;

  ttglyph = ExtractGlyphOutline(ffile, glyph_index, c);

  /*
   * Convert the glyph outline information from TrueType layout into a more
   * easily processed format
   */

  glyph = ConvertOutlineToGlyph(ffile, ttglyph);
  glyph->c = c;
  glyph->glyph_index = *glyph_index;
  glyph->myMetrics = ttglyph->myMetrics;

  /* Free up outline information */
  
  if (ttglyph)
  {
    if (ttglyph->y) POV_FREE(ttglyph->y);
    if (ttglyph->x) POV_FREE(ttglyph->x);
    if (ttglyph->endPoints) POV_FREE(ttglyph->endPoints);
    if (ttglyph->flags) POV_FREE(ttglyph->flags);

    POV_FREE(ttglyph);
  }
  
#ifdef TTF_DEBUG3
    int i, j;

    Debug_Info("// Character '%c'\n", (char)c);

    for(i = 0; i < (int)glyph->header.numContours; i++)
    {
        Debug_Info("BYTE gGlypthFlags_%c_%d[] = \n", (char)c, i);
        Debug_Info("{");
        for(j = 0; j <= (int)glyph->contours[i].count; j++)
        {
            if((j % 10) == 0)
                Debug_Info("\n\t");
            Debug_Info("0x%x, ", (unsigned int)glyph->contours[i].flags[j]);
        }
        Debug_Info("\n};\n\n");

        Debug_Info("DBL gGlypthX_%c_%d[] = \n", (char)c, i);
        Debug_Info("{");
        for(j = 0; j <= (int)glyph->contours[i].count; j++)
        {
            if((j % 10) == 0)
                Debug_Info("\n\t");
            Debug_Info("%f, ", (DBL)glyph->contours[i].x[j]);
        }
        Debug_Info("\n};\n\n");

        Debug_Info("DBL gGlypthY_%c_%d[] = \n", (char)c, i);
        Debug_Info("{");
        for(j = 0; j <= (int)glyph->contours[i].count; j++)
        {
            if((j % 10) == 0)
                Debug_Info("\n\t");
            Debug_Info("%f, ", (DBL)glyph->contours[i].y[j]);
        }
        Debug_Info("\n};\n\n");
    }

    Debug_Info("Contour gGlypthContour_%c[] = \n", (char)c);
    Debug_Info("{\n");
    for(i = 0; i < glyph->header.numContours; i++)
    {
        Debug_Info("\t{\n");
        Debug_Info("\t\t%u, // inside_flag \n", (unsigned int)glyph->contours[i].inside_flag);
        Debug_Info("\t\t%u, // count \n", (unsigned int)glyph->contours[i].count);
        Debug_Info("\t\tgGlypthFlags_%c_%d, // flags[]\n", (char)c, i);
        Debug_Info("\t\tgGlypthX_%c_%d, // x[]\n", (char)c, i);
        Debug_Info("\t\tgGlypthY_%c_%d // y[]\n", (char)c, i);
        Debug_Info("\t},\n");
    }
    Debug_Info("\n};\n\n");

    Debug_Info("Glyph gGlypth_%c = \n", (char)c);
    Debug_Info("{\n");
    Debug_Info("\t{ // header\n");
    Debug_Info("\t\t%u, // header.numContours \n", (unsigned int)glyph->header.numContours);
    Debug_Info("\t\t%f, // header.xMin\n", (DBL)glyph->header.xMin);
    Debug_Info("\t\t%f, // header.yMin\n", (DBL)glyph->header.yMin);
    Debug_Info("\t\t%f, // header.xMax\n", (DBL)glyph->header.xMax);
    Debug_Info("\t\t%f // header.yMax\n", (DBL)glyph->header.yMax);
    Debug_Info("\t},\n");
    Debug_Info("\t%u, // glyph_index\n", (unsigned int)glyph->glyph_index);
    Debug_Info("\tgGlypthContour_%c, // contours[]\n", (char)c);
    Debug_Info("\t%u, // unitsPerEm\n", (unsigned int)glyph->unitsPerEm);
    Debug_Info("\tNULL, // next\n");
    Debug_Info("\t%u, // c\n", (unsigned int)glyph->c);
    Debug_Info("\t%u // myMetrics\n", (unsigned int)glyph->myMetrics);

    Debug_Info("};\n\n");
#endif

  return glyph;
}



/*****************************************************************************
*
* FUNCTION
*
*   ExtractGlyphOutline
*
* INPUT
*   
* OUTPUT
*   
* RETURNS
*   
* AUTHOR
*
*   POV-Ray Team
*   
* DESCRIPTION
*
*   Read the contour information for a specific glyph.  This has to be a
*   separate routine from ExtractGlyphInfo because we call it recurisvely
*   for multiple component glyphs.
*
* CHANGES
*
*   -
*
******************************************************************************/
static GlyphOutline *ExtractGlyphOutline(FontFileInfo *ffile, unsigned int *glyph_index, unsigned int c)
{
  int i;
  USHORT n;
  SHORT nc;
  GlyphOutline *ttglyph;

  ttglyph = (GlyphOutline *)POV_CALLOC(1, sizeof(GlyphOutline), "ttf");
  ttglyph->myMetrics = *glyph_index;

  /* Have to treat space characters differently */
  if (c != ' ')
  {
    ffile->fp->seekg (ffile->glyf_table_offset+ffile->loca_table[*glyph_index]);

    ttglyph->header.numContours = READSHORT(ffile->fp);
    ttglyph->header.xMin = READFWORD(ffile->fp);   /* These may be  */
    ttglyph->header.yMin = READFWORD(ffile->fp);   /* unreliable in */
    ttglyph->header.xMax = READFWORD(ffile->fp);   /* some fonts.   */
    ttglyph->header.yMax = READFWORD(ffile->fp);
  }
  
#ifdef TTF_DEBUG
  Debug_Info("ttglyph->header:\n");
  Debug_Info("glyph_index=%d\n", *glyph_index);
  Debug_Info("loca_table[%d]=%d\n",*glyph_index,ffile->loca_table[*glyph_index]);
  Debug_Info("numContours=%d\n", (int)ttglyph->header.numContours);
#endif

  nc = ttglyph->header.numContours;
  
  /*
   * A positive number of contours means a regular glyph, with possibly
   * several separate line segments making up the outline.
   */
  if (nc > 0)
  {
    FWord coord;
    BYTE flag, repeat_count;
    USHORT temp16;

    /* Grab the contour endpoints */
  
    ttglyph->endPoints = (USHORT *)POV_MALLOC(nc * sizeof(USHORT), "ttf");
  
    for (i = 0; i < nc; i++)
    {
      ttglyph->endPoints[i] = READUSHORT(ffile->fp);
#ifdef TTF_DEBUG
      Debug_Info("endPoints[%d]=%d\n", i, ttglyph->endPoints[i]);
#endif
    }

    /* Skip over the instructions */
    temp16 = READUSHORT(ffile->fp);
    ffile->fp->seekg (temp16, POV_SEEK_CUR);
#ifdef TTF_DEBUG
    Debug_Info("skipping instruction bytes: %d\n", temp16);
#endif

    /* Determine the number of points making up this glyph */
    
    n = ttglyph->numPoints = ttglyph->endPoints[nc - 1] + 1;
#ifdef TTF_DEBUG
    Debug_Info("numPoints=%d\n", ttglyph->numPoints);
#endif

    /* Read the flags */
    
    ttglyph->flags = (BYTE *)POV_MALLOC(n * sizeof(BYTE), "ttf");
    
    for (i = 0; i < ttglyph->numPoints; i++)
    {
      if (!ffile->fp->read((char *)(&ttglyph->flags[i]), sizeof(BYTE)))
      {
        Error("Cannot read TrueType font file at line %d, %s",
              __LINE__, __FILE__);
      }
      
      if (ttglyph->flags[i] & REPEAT_FLAGS)
      {
        if (!ffile->fp->read((char *)(&repeat_count), sizeof(BYTE)))
        {
          Error("Cannot read TrueType font file at line %d, %s",
                __LINE__, __FILE__);
        }
        for (; repeat_count > 0; repeat_count--, i++)
        {
#ifdef TTF_DEBUG
              if (i>=n)
              {
                Debug_Info("readflags ERROR: i >= n (%d > %d)\n", i, n);
                }
#endif
          if (i<n)      /* hack around a bug that is trying to write too many flags */
            ttglyph->flags[i + 1] = ttglyph->flags[i];
        }
      }
    }
#ifdef  TTF_DEBUG
    Debug_Info("flags:");
    for (i=0; i<n; i++)
      Debug_Info(" %02x", ttglyph->flags[i]);
    Debug_Info("\n");
#endif
    /* Read the coordinate vectors */
    
    ttglyph->x = (DBL *)POV_MALLOC(n * sizeof(DBL), "ttf");
    ttglyph->y = (DBL *)POV_MALLOC(n * sizeof(DBL), "ttf");

    coord = 0;
    
    for (i = 0; i < ttglyph->numPoints; i++)
    {
      /* Read each x coordinate */
      
      flag = ttglyph->flags[i];
      
      if (flag & XSHORT)
      {
        BYTE temp8;

        if (!ffile->fp->read((char *)(&temp8), 1))
        {
          Error("Cannot read TrueType font file at line %d, %s",
                __LINE__, __FILE__);
        }
        
        if (flag & SHORT_X_IS_POS)
          coord += temp8;
        else
          coord -= temp8;
      }
      else if (!(flag & NEXT_X_IS_ZERO))
      {
        coord += READSHORT(ffile->fp);
      }
      
      /* Find our own maximum and minimum x coordinates */
      if (coord > ttglyph->header.xMax)
        ttglyph->header.xMax = coord;
      if (coord < ttglyph->header.xMin)
        ttglyph->header.xMin = coord;

      ttglyph->x[i] = (DBL)coord / (DBL)ffile->unitsPerEm;
    }

    coord = 0;
    
    for (i = 0; i < ttglyph->numPoints; i++)
    {
      /* Read each y coordinate */
      
      flag = ttglyph->flags[i];
      
      if (flag & YSHORT)
      {
        BYTE temp8;

        if (!ffile->fp->read((char *)(&temp8), 1))
        {
          Error("Cannot read TrueType font file at line %d, %s",
                __LINE__, __FILE__);
        }
        
        if (flag & SHORT_Y_IS_POS)
          coord += temp8;
        else
          coord -= temp8;
      }
      else if (!(flag & NEXT_Y_IS_ZERO))
      {
        coord += READSHORT(ffile->fp);
      }
      
      /* Find out our own maximum and minimum y coordinates */
      if (coord > ttglyph->header.yMax)
        ttglyph->header.yMax = coord;
      if (coord < ttglyph->header.yMin)
        ttglyph->header.yMin = coord;

      ttglyph->y[i] = (DBL)coord / (DBL)ffile->unitsPerEm;
    }
  }
  /*
   * A negative number for numContours means that this glyph is
   * made up of several separate glyphs.
   */
  else if (nc < 0)
  {
    USHORT flags;

    ttglyph->header.numContours = 0;
    ttglyph->numPoints = 0;

    do
    {
      GlyphOutline *sub_ttglyph;
      unsigned int sub_glyph_index;
      int   current_pos;
      SHORT arg1, arg2;
      DBL xoff = 0, yoff = 0;
      DBL xscale = 1, yscale = 1;
      DBL scale01 = 0, scale10 = 0;
      USHORT n2;
      SHORT nc2;

      flags = READUSHORT(ffile->fp);
      sub_glyph_index = READUSHORT(ffile->fp);

#ifdef TTF_DEBUG
      Debug_Info("sub_glyph %d: ", sub_glyph_index);
#endif
   
      if (flags & ARG_1_AND_2_ARE_WORDS)
      {
#ifdef TTF_DEBUG
        Debug_Info("ARG_1_AND_2_ARE_WORDS ");
#endif
        arg1 = READSHORT(ffile->fp);
        arg2 = READSHORT(ffile->fp);
      }
      else
      {
        arg1 = READUSHORT(ffile->fp);
        arg2 = arg1 & 0xFF;
        arg1 = (arg1 >> 8) & 0xFF;
      }

#ifdef TTF_DEBUG
      if (flags & ROUND_XY_TO_GRID)
      {
        Debug_Info("ROUND_XY_TO_GRID ");
      }

      if (flags & MORE_COMPONENTS)
      {
        Debug_Info("MORE_COMPONENTS ");
      }
#endif

      if (flags & WE_HAVE_A_SCALE)
      {
        xscale = yscale = (DBL)READSHORT(ffile->fp)/0x4000;
#ifdef TTF_DEBUG
        Debug_Info("WE_HAVE_A_SCALE ");
        Debug_Info("xscale = %lf\t", xscale);
        Debug_Info("scale01 = %lf\n", scale01);
        Debug_Info("scale10 = %lf\t", scale10);
        Debug_Info("yscale = %lf\n", yscale);
#endif
      }
      else if (flags & WE_HAVE_AN_X_AND_Y_SCALE)
      {
        xscale = (DBL)READSHORT(ffile->fp)/0x4000;
        yscale = (DBL)READSHORT(ffile->fp)/0x4000;
#ifdef TTF_DEBUG
        Debug_Info("WE_HAVE_AN_X_AND_Y_SCALE ");
        Debug_Info("xscale = %lf\t", xscale);
        Debug_Info("scale01 = %lf\n", scale01);
        Debug_Info("scale10 = %lf\t", scale10);
        Debug_Info("yscale = %lf\n", yscale);
#endif
      }
      else if (flags & WE_HAVE_A_TWO_BY_TWO)
      {
        xscale  = (DBL)READSHORT(ffile->fp)/0x4000;
        scale01 = (DBL)READSHORT(ffile->fp)/0x4000;
        scale10 = (DBL)READSHORT(ffile->fp)/0x4000;
        yscale  = (DBL)READSHORT(ffile->fp)/0x4000;
#ifdef TTF_DEBUG
        Debug_Info("WE_HAVE_A_TWO_BY_TWO ");
        Debug_Info("xscale = %lf\t", xscale);
        Debug_Info("scale01 = %lf\n", scale01);
        Debug_Info("scale10 = %lf\t", scale10);
        Debug_Info("yscale = %lf\n", yscale);
#endif
      }

      if (flags & ARGS_ARE_XY_VALUES)
      {
        xoff = (DBL)arg1 / ffile->unitsPerEm;
        yoff = (DBL)arg2 / ffile->unitsPerEm;

#ifdef TTF_DEBUG
        Debug_Info("ARGS_ARE_XY_VALUES ");
        Debug_Info("\narg1 = %d  xoff = %lf\t", arg1, xoff);
        Debug_Info("arg2 = %d  yoff = %lf\n", arg2, yoff);
#endif
      }
      else  /* until I understand how this method works... */
      {
        Warning(0, "Cannot handle part of glyph %d (0x%X).", c, c);
        continue;
      }

      if (flags & USE_MY_METRICS)
      {
#ifdef TTF_DEBUG
        Debug_Info("USE_MY_METRICS ");
#endif
        ttglyph->myMetrics = sub_glyph_index;
      }

      current_pos = ffile->fp->tellg () ;
      sub_ttglyph = ExtractGlyphOutline(ffile, &sub_glyph_index, c);
      ffile->fp->seekg (current_pos) ;

      if ((nc2 = sub_ttglyph->header.numContours) == 0)
        continue;

      nc = ttglyph->header.numContours;
      n = ttglyph->numPoints;
      n2 = sub_ttglyph->numPoints;

      ttglyph->endPoints = (USHORT *)POV_REALLOC(ttglyph->endPoints,
                                       (nc + nc2) * sizeof(USHORT), "ttf");
      ttglyph->flags = (BYTE *)POV_REALLOC(ttglyph->flags, (n+n2)*sizeof(BYTE), "ttf");
      ttglyph->x = (DBL *)POV_REALLOC(ttglyph->x, (n + n2) * sizeof(DBL), "ttf");
      ttglyph->y = (DBL *)POV_REALLOC(ttglyph->y, (n + n2) * sizeof(DBL), "ttf");

      /* Add the sub glyph info to the end of the current glyph */

      ttglyph->header.numContours += nc2;
      ttglyph->numPoints += n2;

      for (i = 0; i < nc2; i++)
      {
        ttglyph->endPoints[i + nc] = sub_ttglyph->endPoints[i] + n;
#ifdef TTF_DEBUG
        Debug_Info("endPoints[%d]=%d\n", i + nc, ttglyph->endPoints[i + nc]);
#endif
      }

      for (i = 0; i < n2; i++)
      {
#ifdef TTF_DEBUG
        Debug_Info("x[%d]=%lf\t", i, sub_ttglyph->x[i]);
        Debug_Info("y[%d]=%lf\n", i, sub_ttglyph->y[i]);
#endif
        ttglyph->flags[i + n] = sub_ttglyph->flags[i];
        ttglyph->x[i + n] = xscale * sub_ttglyph->x[i] +
                            scale01 * sub_ttglyph->y[i] + xoff;
        ttglyph->y[i + n] = scale10 * sub_ttglyph->x[i] +
                            yscale * sub_ttglyph->y[i] + yoff;

#ifdef TTF_DEBUG
        Debug_Info("x[%d]=%lf\t", i+n, ttglyph->x[i+n]);
        Debug_Info("y[%d]=%lf\n", i+n, ttglyph->y[i+n]);
#endif

        if (ttglyph->x[i + n] < ttglyph->header.xMin)
          ttglyph->header.xMin = ttglyph->x[i + n];

        if (ttglyph->x[i + n] > ttglyph->header.xMax)
          ttglyph->header.xMax = ttglyph->x[i + n];

        if (ttglyph->y[i + n] < ttglyph->header.yMin)
          ttglyph->header.yMin = ttglyph->y[i + n];

        if (ttglyph->y[i + n] > ttglyph->header.yMax)
          ttglyph->header.yMax = ttglyph->y[i + n];
      }

      /* Free up the sub glyph outline information */
  
      if (sub_ttglyph->y) POV_FREE(sub_ttglyph->y);
      if (sub_ttglyph->x) POV_FREE(sub_ttglyph->x);
      if (sub_ttglyph->endPoints) POV_FREE(sub_ttglyph->endPoints);
      if (sub_ttglyph->flags) POV_FREE(sub_ttglyph->flags);

      POV_FREE(sub_ttglyph);
    } while (flags & MORE_COMPONENTS);
  }

#ifdef TTF_DEBUG
    Debug_Info("xMin=%d\n",ttglyph->header.xMin);
    Debug_Info("yMin=%d\n",ttglyph->header.yMin);
    Debug_Info("xMax=%d\n",ttglyph->header.xMax);
    Debug_Info("yMax=%d\n",ttglyph->header.yMax);
#endif

  return ttglyph;
}



/*****************************************************************************
*
* FUNCTION
*
*   ConvertOutlineToGlyph
*
* INPUT
*   
* OUTPUT
*   
* RETURNS
*   
* AUTHOR
*
*   POV-Ray Team
*   
* DESCRIPTION
*
* Transform a glyph from TrueType storage format to something a little easier
* to manage.
*
* CHANGES
*
*   -
*
******************************************************************************/
static GlyphPtr ConvertOutlineToGlyph(FontFileInfo *ffile, GlyphOutline *ttglyph)
{
  GlyphPtr glyph;
  DBL *temp_x, *temp_y;
  BYTE *temp_f;
  USHORT i, j, last_j;

  /* Create storage for this glyph */

  glyph = (Glyph *)POV_MALLOC(sizeof(Glyph), "ttf");
  if (ttglyph->header.numContours > 0)
  {
    glyph->contours = (Contour *)POV_MALLOC(ttglyph->header.numContours * sizeof(Contour), "ttf");
  }
  else
  {
    glyph->contours = NULL;
  }

  /* Copy sizing information about this glyph */
  
  POV_MEMCPY(&glyph->header, &ttglyph->header, sizeof(GlyphHeader));

  /* Keep track of the size for this glyph */
  
  glyph->unitsPerEm = ffile->unitsPerEm;

  /* Now copy the vertex information into the contours */
  
  for (i = 0, last_j = 0; i < (USHORT) ttglyph->header.numContours; i++)
  {
    /* Figure out number of points in contour */
  
    j = ttglyph->endPoints[i] - last_j + 1;

    /* Copy the coordinate information into the glyph */
  
    temp_x = (DBL *)POV_MALLOC((j + 1) * sizeof(DBL), "ttf");
    temp_y = (DBL *)POV_MALLOC((j + 1) * sizeof(DBL), "ttf");

    temp_f = (BYTE *)POV_MALLOC((j + 1) * sizeof(BYTE), "ttf");
    POV_MEMCPY(temp_x, &ttglyph->x[last_j], j * sizeof(DBL));
    POV_MEMCPY(temp_y, &ttglyph->y[last_j], j * sizeof(DBL));

    POV_MEMCPY(temp_f, &ttglyph->flags[last_j], j * sizeof(BYTE));
    temp_x[j] = ttglyph->x[last_j];
    temp_y[j] = ttglyph->y[last_j];
    temp_f[j] = ttglyph->flags[last_j];

    /* Figure out if this is an inside or outside contour */
    
    glyph->contours[i].inside_flag = 0;

    /* Plug in the reset of the contour components into the glyph */
    
    glyph->contours[i].count = j;
    glyph->contours[i].x = temp_x;
    glyph->contours[i].y = temp_y;
    glyph->contours[i].flags = temp_f;

    /*
     * Set last_j to point to the beginning of the next contour's coordinate
     * information
     */
    
    last_j = ttglyph->endPoints[i] + 1;
  }

  /* Show statistics about this glyph */

#ifdef TTF_DEBUG
   Debug_Info("Number of contours: %u\n", glyph->header.numContours);
   Debug_Info("X extent: [%f, %f]\n",
     (DBL)glyph->header.xMin / (DBL)ffile->unitsPerEm,
     (DBL)glyph->header.xMax / (DBL)ffile->unitsPerEm);

   Debug_Info("Y extent: [%f, %f]\n",
     (DBL)glyph->header.yMin / (DBL)ffile->unitsPerEm,
     (DBL)glyph->header.yMax / (DBL)ffile->unitsPerEm);

   Debug_Info("Converted coord list(%d):\n", (int)glyph->header.numContours);

   for (i=0;i<(USHORT)glyph->header.numContours;i++)
   {
     for (j=0;j<=glyph->contours[i].count;j++)
       Debug_Info("  %c[%f, %f]\n",
         (glyph->contours[i].flags[j] & ONCURVE ? '*' : ' '),
         glyph->contours[i].x[j], glyph->contours[i].y[j]);
     Debug_Info("\n");
   }
#endif

  return glyph;
}

/* Test to see if "point" is inside the splined polygon "points". */
static int Inside_Glyph(double x, double y, GlyphPtr glyph)
{
  int i, j, k, n, n1, crossings;
  int qi, ri, qj, rj;
  Contour *contour;
  double xt[3], yt[3], roots[2];
  DBL *xv, *yv;
  double x0, x1, x2, t;
  double y0, y1, y2;
  double m, b, xc;
  BYTE *fv;

  crossings = 0;
  
  n = glyph->header.numContours;
  
  contour = glyph->contours;
  
  for (i = 0; i < n; i++)
  {
    xv = contour[i].x;
    yv = contour[i].y;
    fv = contour[i].flags;
    x0 = xv[0];
    y0 = yv[0];
    n1 = contour[i].count;
  
    for (j = 1; j <= n1; j++)
    {
      x1 = xv[j];
      y1 = yv[j];
      
      if (fv[j] & ONCURVE)
      {
        /* Straight line - first set up for the next */
        /* Now do the crossing test */
        
        qi = ri = qj = rj = 0;
        
        if (y0 == y1)
          goto end_line_test;
        
        /* if (fabs((y - y0) / (y1 - y0)) < EPSILON) goto end_line_test; */
        
        if (y0 < y)
          qi = 1;
        
        if (y1 < y)
          qj = 1;
        
        if (qi == qj)
          goto end_line_test;
        
        if (x0 > x)
          ri = 1;
        
        if (x1 > x)
          rj = 1;
        
        if (ri & rj)
        {
          crossings++;
          goto end_line_test;
        }
        
        if ((ri | rj) == 0)
          goto end_line_test;
        
        m = (y1 - y0) / (x1 - x0);
        b = (y1 - y) - m * (x1 - x);
        
        if ((b / m) < EPSILON)
        {
          crossings++;
        }
      
      end_line_test:
        x0 = x1;
        y0 = y1;
      }
      else
      {
        if (j == n1)
        {
          x2 = xv[0];
          y2 = yv[0];
        }
        else
        {
          x2 = xv[j + 1];
          y2 = yv[j + 1];
      
          if (!(fv[j + 1] & ONCURVE))
          {
            /*
             * Parabola with far end floating - readjust the far end so that it
             * is on the curve.
             */
            
            x2 = 0.5 * (x1 + x2);
            y2 = 0.5 * (y1 + y2);
          }
        }

        /* only test crossing when y is in the range */
        /* this should also help saving some computations */
        if (((y0 < y) && (y1 < y) && (y2 < y)) ||
            ((y0 > y) && (y1 > y) && (y2 > y)))
          goto end_curve_test;

        yt[0] = y0 - 2.0 * y1 + y2;
        yt[1] = 2.0 * (y1 - y0);
        yt[2] = y0 - y;
        
        k = solve_quad(yt, roots, 0.0, 1.0);

        for (ri = 0; ri < k;) {
          if (roots[ri] <= EPSILON) {
            /* if y actually is not in range, discard the root */
            if (((y <= y0) && (y < y1)) || ((y >= y0) && (y > y1))) {
              k--;
              if (k > ri)
                roots[ri] = roots[ri+1];
              continue;
            }
          }
         else if (roots[ri] >= (1.0 - EPSILON)) {
            /* if y actually is not in range, discard the root */
            if (((y < y2) && (y < y1)) || ((y > y2) && (y > y1))) {
              k--;
              if (k > ri)
                roots[ri] = roots[ri+1];
              continue;
            }
          }

          ri++;
        }
        
        if (k > 0)
        {
          xt[0] = x0 - 2.0 * x1 + x2;
          xt[1] = 2.0 * (x1 - x0);
          xt[2] = x0;
        
          t = roots[0];
          
          xc = (xt[0] * t + xt[1]) * t + xt[2];
          
          if (xc > x)
            crossings++;
          
          if (k > 1)
          {
            t = roots[1];
            xc = (xt[0] * t + xt[1]) * t + xt[2];
          
            if (xc > x)
              crossings++;
          }
        }

end_curve_test:
        
        x0 = x2;
        
        y0 = y2;
      }
    }
  }
  
  return (crossings & 1);
}


static int solve_quad(double *x, double *y, double mindist, DBL maxdist)
{
  double d, t, a, b, c, q;

  a = x[0];
  b = -x[1];
  c = x[2];
  
  if (fabs(a) < COEFF_LIMIT)
  {
    if (fabs(b) < COEFF_LIMIT)
      return 0;
  
    q = c / b;
  
    if (q >= mindist && q <= maxdist)
    {
      y[0] = q;
      return 1;
    }
    else
      return 0;
  }
  
  d = b * b - 4.0 * a * c;

  if (d < EPSILON)
    return 0;

  d = sqrt(d);
  t = 2.0 * a;
  q = (b + d) / t;
  
  if (q >= mindist && q <= maxdist)
  {
    y[0] = q;
    q = (b - d) / t;
    
    if (q >= mindist && q <= maxdist)
    {
      y[1] = q;
      return 2;
    }
    
    return 1;
  }
  
  q = (b - d) / t;
  
  if (q >= mindist && q <= maxdist)
  {
    y[0] = q;
    return 1;
  }
  
  return 0;
}

/*
 * Returns the distance to z = 0 in t0, and the distance to z = 1 in t1.
 * These distances are to the the bottom and top surfaces of the glyph.
 * The distances are set to -1 if there is no hit.
 */
static void GetZeroOneHits(GlyphPtr glyph, VECTOR P, VECTOR D, DBL glyph_depth, double *t0, double *t1)
{
  double x0, y0, t;

  *t0 = -1.0;
  *t1 = -1.0;

  /* Are we parallel to the x-y plane? */
  
  if (fabs(D[Z]) < EPSILON)
    return;

  /* Solve: P[Y] + t * D[Y] = 0 */
  
  t = -P[Z] / D[Z];
  
  x0 = P[X] + t * D[X];
  y0 = P[Y] + t * D[Y];
  
  if (Inside_Glyph(x0, y0, glyph))
    *t0 = t;

  /* Solve: P[Y] + t * D[Y] = glyph_depth */
  
  t += (glyph_depth / D[Z]);
  
  x0 = P[X] + t * D[X];
  y0 = P[Y] + t * D[Y];
  
  if (Inside_Glyph(x0, y0, glyph))
    *t1 = t;
}

/*
 * Solving for a linear sweep of a non-linear curve can be performed by
 * projecting the ray onto the x-y plane, giving a parametric equation for the
 * ray as:
 * 
 * x = x0 + x1 t, y = y0 + y1 t
 * 
 * Eliminating t from the above gives the implicit equation:
 * 
 * y1 x - x1 y - (x0 y1 - y0 x1) = 0.
 * 
 * Substituting a parametric equation for x and y gives:
 * 
 * y1 x(s) - x1 y(s) - (x0 y1 - y0 x1) = 0.
 * 
 * which can be written as
 * 
 * a x(s) + b y(s) + c = 0,
 * 
 * where a = y1, b = -x1, c = (y0 x1 - x0 y1).
 * 
 * For piecewise quadratics, the parametric equations will have the forms:
 * 
 * x(s) = (1-s)^2 P0(x) + 2 s (1 - s) P1(x) + s^2 P2(x) y(s) = (1-s)^2 P0(y) + 2 s
 * (1 - s) P1(y) + s^2 P2(y)
 * 
 * where P0 is the first defining vertex of the spline, P1 is the second, P2 is
 * the third.  Using the substitutions:
 * 
 * xt2 = x0 - 2 x1 + x2, xt1 = 2 * (x1 - x0), xt0 = x0; yt2 = y0 - 2 y1 + y2, yt1
 * = 2 * (y1 - y0), yt0 = y0;
 * 
 * the equations can be written as:
 * 
 * x(s) = xt2 s^2 + xt1 s + xt0, y(s) = yt2 s^2 + yt1 s + yt0.
 * 
 * Substituting and multiplying out gives the following equation in s:
 * 
 * s^2 * (a*xt2 + b*yt2) + s   * (a*xt1 + b*yt1) + c + a*xt0 + b*yt0
 * 
 * This is then solved using the quadratic formula.  Any solutions of s that are
 * between 0 and 1 (inclusive) are valid solutions.
 */
static int GlyphIntersect(OBJECT *Object, VECTOR P, VECTOR D, GlyphPtr glyph, DBL glyph_depth,
                          RAY *Ray, ISTACK *Depth_Stack) /* tw */
{
  Contour *contour;
  int i, j, k, l, n, m, Flag = 0;
  VECTOR N, IPoint;
  DBL Depth;
  double x0, x1, y0, y1, x2, y2, t, t0, t1, z;
  double xt0, xt1, xt2, yt0, yt1, yt2;
  double a, b, c, d0, d1, C[3], S[2];
  DBL *xv, *yv;
  BYTE *fv;
  TTF *ttf = (TTF *) Object;
  int dirflag = 0;

  /*
   * First thing to do is to get any hits at z = 0 and z = 1 (which are the
   * bottom and top surfaces of the glyph.
   */
  
  GetZeroOneHits(glyph, P, D, glyph_depth, &t0, &t1);

  if (t0 > 0.0)
  {
    Depth = t0 /* / len */;
    VScale(IPoint, Ray->Direction, Depth);
    VAddEq(IPoint, Ray->Initial);
  
    if (Depth > TTF_Tolerance &&
        Point_In_Clip(IPoint, Object->Clip))
    {
      Make_Vector(N, 0.0, 0.0, -1.0);
      MTransNormal(N, N, ttf->Trans);
      VNormalize(N, N);
      push_normal_entry(Depth, IPoint, N, Object, Depth_Stack);
      Flag = true;
    }
  }

  if (t1 > 0.0)
  {
    Depth = t1 /* / len */;
    VScale(IPoint, Ray->Direction, Depth);
    VAddEq(IPoint, Ray->Initial);
    
    if (Depth > TTF_Tolerance &&
        Point_In_Clip(IPoint, Object->Clip))
    {
      Make_Vector(N, 0.0, 0.0, 1.0);
      MTransNormal(N, N, ttf->Trans);
      VNormalize(N, N);
      push_normal_entry(Depth, IPoint, N, Object, Depth_Stack);
      Flag = true;
    }
  }

  /* Simple test to see if we can just toss this ray */
  
  if (fabs(D[X]) < EPSILON)
  {
    if (fabs(D[Y]) < EPSILON)
    {
      /*
       * This means the ray is moving parallel to the walls of the sweep
       * surface
       */
      return Flag;
    }
    else
    {
      dirflag = 0;
    }
  }
  else
  {
    dirflag = 1;
  }

  /*
   * Now walk through the glyph, looking for places where the ray hits the
   * walls
   */
  
  a = D[Y];
  b = -D[X];
  c = (P[Y] * D[X] - P[X] * D[Y]);

  n = glyph->header.numContours;
  
  for (i = 0, contour = glyph->contours; i < n; i++, contour++)
  {
    xv = contour->x;
    yv = contour->y;
    fv = contour->flags;
    x0 = xv[0];
    y0 = yv[0];
    m = contour->count;
  
    for (j = 1; j <= m; j++)
    {
      x1 = xv[j];
      y1 = yv[j];
    
      if (fv[j] & ONCURVE)
      {
        /* Straight line */
        d0 = (x1 - x0);
        d1 = (y1 - y0);

        t0 = d1 * D[X] - d0 * D[Y];
        
        if (fabs(t0) < EPSILON)
          /* No possible intersection */
          goto end_line_test;
        
        t = (D[X] * (P[Y] - y0) - D[Y] * (P[X] - x0)) / t0;
        
        if (t < 0.0 || t > 1.0)
          goto end_line_test;
        
        if (dirflag)
          t = ((x0 + t * d0) - P[X]) / D[X];
        else
          t = ((y0 + t * d1) - P[Y]) / D[Y];
        
        z = P[Z] + t * D[Z];
        
        Depth = t /* / len */;

        if (z >= 0 && z <= glyph_depth && Depth > TTF_Tolerance)
        {
          VScale(IPoint, Ray->Direction, Depth);
          VAddEq(IPoint, Ray->Initial);
        
          if (Point_In_Clip(IPoint, Object->Clip))
          {
            Make_Vector(N, d1, -d0, 0.0);
            MTransNormal(N, N, ttf->Trans);
            VNormalize(N, N);
            push_normal_entry(Depth, IPoint, N, Object, Depth_Stack);
            Flag = true;
          }
        }
      end_line_test:
        x0 = x1;
        y0 = y1;
      }
      else
      {
        if (j == m)
        {
          x2 = xv[0];
          y2 = yv[0];
        }
        else
        {
          x2 = xv[j + 1];
          y2 = yv[j + 1];
          
          if (!(fv[j + 1] & ONCURVE))
          {

            /*
             * Parabola with far end DBLing - readjust the far end so that it
             * is on the curve.  (In the correct place too.)
             */
            
            x2 = 0.5 * (x1 + x2);
            y2 = 0.5 * (y1 + y2);
          }
        }

        /* Make the interpolating quadrics */
        
        xt2 = x0 - 2.0 * x1 + x2;
        xt1 = 2.0 * (x1 - x0);
        xt0 = x0;
        yt2 = y0 - 2.0 * y1 + y2;
        yt1 = 2.0 * (y1 - y0);
        yt0 = y0;

        C[0] = a * xt2 + b * yt2;
        C[1] = a * xt1 + b * yt1;
        C[2] = a * xt0 + b * yt0 + c;

        k = solve_quad(C, S, 0.0, 1.0);

        for (l = 0; l < k; l++)
        {
          if (dirflag)
            t = ((S[l] * S[l] * xt2 + S[l] * xt1 + xt0) - P[X]) / D[X];
          else
            t = ((S[l] * S[l] * yt2 + S[l] * yt1 + yt0) - P[Y]) / D[Y];

          /*
           * If the intersection with this wall is between 0 and glyph_depth
           * along the z-axis, then it is a valid hit.
           */

          z = P[Z] + t * D[Z];
          
          Depth = t /* / len */;
          
          if (z >= 0 && z <= glyph_depth && Depth > TTF_Tolerance)
          {
            VScale(IPoint, Ray->Direction, Depth);
            VAddEq(IPoint, Ray->Initial);
            
            if (Point_In_Clip(IPoint, Object->Clip))
            {
              Make_Vector(N, 2.0 * yt2 * S[l] + yt1, -2.0 * xt2 * S[l] - xt1, 0.0);
              MTransNormal(N, N, ttf->Trans);
              VNormalize(N, N);
              push_normal_entry(Depth, IPoint, N, Object, Depth_Stack);
              Flag = true;
            }
          }
        }
        
        x0 = x2;
        y0 = y2;
      }
    }
  }
  
  return Flag;
}

static int All_TTF_Intersections(OBJECT *Object, RAY *Ray, ISTACK *Depth_Stack)
{
  TTF *ttf = (TTF *) Object;
  VECTOR P, D;
  GlyphPtr glyph;

  Increase_Counter(stats[Ray_TTF_Tests]);

  /* Transform the point into the glyph's space */
  
  MInvTransPoint(P, Ray->Initial, ttf->Trans);
  MInvTransDirection(D, Ray->Direction, ttf->Trans);

  /* Tweak the ray to try to avoid pathalogical intersections */
/*  DBL len;

  D[0] *= 1.0000013147;
  D[1] *= 1.0000022741;
  D[2] *= 1.0000017011;

  VLength(len, D);
  VInverseScaleEq(D, len);*/

  glyph = (GlyphPtr)ttf->glyph;

  if (GlyphIntersect(Object, P, D, glyph,ttf->depth,Ray,Depth_Stack)) /* tw */
  {
    Increase_Counter(stats[Ray_TTF_Tests_Succeeded]);
    return true;
  }

  return false;
}

static int Inside_TTF(VECTOR IPoint, OBJECT *Object)
{
  VECTOR New_Point;
  TTF *ttf = (TTF *) Object;
  GlyphPtr glyph;

  /* Transform the point into font space */
  
  MInvTransPoint(New_Point, IPoint, ttf->Trans);

  glyph = (GlyphPtr)ttf->glyph;

  if (New_Point[Z] >= 0.0 && New_Point[Z] <= ttf->depth &&
      Inside_Glyph(New_Point[X], New_Point[Y], glyph))
    return (!Test_Flag(ttf, INVERTED_FLAG));
  else
    return (Test_Flag(ttf, INVERTED_FLAG));
}

static void TTF_Normal(VECTOR Result, OBJECT * /*Object*/, INTERSECTION *Inter)
{
  /* Use precomputed normal. [ARE 11/94] */

  Assign_Vector(Result, Inter->INormal);
}

static TTF *Copy_TTF(OBJECT *Object)
{
  TTF *New, *Old = (TTF *) Object;

  New = Create_TTF();
  Destroy_Transform(New->Trans);

  *New = *Old;

  New->Trans = Copy_Transform(((TTF *) Object)->Trans);
  
  New->glyph = Old->glyph;

  return (New);
}

static void Translate_TTF(OBJECT *Object, VECTOR /*Vector*/, TRANSFORM *Trans)
{
  Transform_TTF(Object, Trans);
}

static void Rotate_TTF(OBJECT *Object, VECTOR /*Vector*/, TRANSFORM *Trans)
{
  Transform_TTF(Object, Trans);
}

static void Scale_TTF(OBJECT *Object, VECTOR /*Vector*/, TRANSFORM *Trans)
{
  Transform_TTF(Object, Trans);
}

static void Invert_TTF(OBJECT *Object)
{
  Invert_Flag(Object, INVERTED_FLAG);
}

static void Transform_TTF(OBJECT *Object, TRANSFORM *Trans)
{
  TTF *ttf = (TTF *) Object;

  Compose_Transforms(ttf->Trans, Trans);

  /* Calculate the bounds */

  Compute_TTF_BBox(ttf);
}

static TTF *Create_TTF()
{
  TTF *New;

  New = (TTF *) POV_MALLOC(sizeof(TTF), "ttf");

  INIT_OBJECT_FIELDS(New, TTF_OBJECT, &TTF_Methods)
  /* Initialize TTF specific information */

  New->Trans = Create_Transform();
  
  New->glyph = NULL;
  New->depth = 1.0;

  /* Default bounds */
  Make_BBox(New->BBox, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0);

  return New;
}

static void Destroy_TTF(OBJECT *Object)
{
  TTF *ttf = (TTF *) Object;

#if(DUMP_OBJECT_DATA == 1)
  Debug_Info("{ // TTF \n");
  DUMP_OBJECT_FIELDS(Object);
  Debug_Info("\tNULL, // glyph\n");
  Debug_Info("\t%f // depth\n", (DBL)ttf->depth);
  Debug_Info("}\n");
  if(Object->Trans != NULL)
  {
    int i;

    Debug_Info("{\n");
    Debug_Info("\t{\n");
    for(i = 0; i <= 3; i++)
    {
      Debug_Info("\t\t{ ");
      Debug_Info("%f, %f, %f, %f",
                 Object->Trans->matrix[i][0],
                 Object->Trans->matrix[i][1],
                 Object->Trans->matrix[i][2],
                 Object->Trans->matrix[i][3]);
      if(i < 3)
        Debug_Info(" },\n");
      else
        Debug_Info(" }\n");
    }
    Debug_Info("\t}\n");

    Debug_Info("\t{\n");
    for(i = 0; i <= 3; i++)
    {
      Debug_Info("\t\t{ ");
      Debug_Info("%f, %f, %f, %f",
                 Object->Trans->inverse[i][0],
                 Object->Trans->inverse[i][1],
                 Object->Trans->inverse[i][2],
                 Object->Trans->inverse[i][3]);
      if(i < 3)
        Debug_Info(" },\n");
      else
        Debug_Info(" }\n");
    }
    Debug_Info("\t}\n");
    Debug_Info("}\n");
  }
#endif

  Destroy_Transform(ttf->Trans);
  POV_FREE(Object);
}



/*****************************************************************************
*
* FUNCTION
*
*   Compute_TTF_BBox
*
* INPUT
*
*   ttf - ttf
*
* OUTPUT
*
*   ttf
*
* RETURNS
*
* AUTHOR
*
*   Dieter Bayer, August 1994
*
* DESCRIPTION
*
*   Calculate the bounding box of a true type font.
*
* CHANGES
*
*   -
*
******************************************************************************/
void Compute_TTF_BBox(TTF *ttf)
{
  DBL funit_size, xMin, yMin, zMin, xMax, yMax, zMax;
  GlyphPtr glyph;

  glyph = (GlyphPtr)ttf->glyph;
  funit_size = 1.0 / (DBL)(glyph->unitsPerEm);

  xMin = (DBL)glyph->header.xMin * funit_size;
  yMin = (DBL)glyph->header.yMin * funit_size;
  zMin = -TTF_Tolerance;

  xMax = (DBL)glyph->header.xMax * funit_size;
  yMax = (DBL)glyph->header.yMax * funit_size;
  zMax = ttf->depth + TTF_Tolerance;

  Make_BBox(ttf->BBox, xMin, yMin, zMin, xMax - xMin, yMax - yMin, zMax - zMin);

#ifdef TTF_DEBUG
  Debug_Info("Bounds: <%g,%g,%g> -> <%g,%g,%g>\n",
             ttf->BBox.Lower_Left[0],
             ttf->BBox.Lower_Left[1],
             ttf->BBox.Lower_Left[2],
             ttf->BBox.Lengths[0],
             ttf->BBox.Lengths[1],
             ttf->BBox.Lengths[2]);
#endif

  /* Apply the transformation to the bounding box */

  Recompute_BBox(&ttf->BBox, ttf->Trans);
}

END_POV_NAMESPACE

