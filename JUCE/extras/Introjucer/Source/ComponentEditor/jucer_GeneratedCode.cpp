/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2015 - ROLI Ltd.

   Permission is granted to use this software under the terms of either:
   a) the GPL v2 (or any later version)
   b) the Affero GPL v3

   Details of these licenses can be found at: www.gnu.org/licenses

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

   ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.juce.com for more information.

  ==============================================================================
*/

#include "../jucer_Headers.h"
#include "jucer_GeneratedCode.h"
#include "jucer_JucerDocument.h"

//==============================================================================
GeneratedCode::GeneratedCode (const JucerDocument* const doc)
    : document (doc), suffix (0)
{
}

GeneratedCode::~GeneratedCode()
{
}

int GeneratedCode::getUniqueSuffix()
{
    return ++suffix;
}

//==============================================================================
String& GeneratedCode::getCallbackCode (const String& requiredParentClass,
                                        const String& returnType,
                                        const String& prototype,
                                        const bool hasPrePostUserSections)
{
    String parentClass (requiredParentClass);
    if (parentClass.isNotEmpty()
         && ! (parentClass.startsWith ("public ")
                || parentClass.startsWith ("private ")
                || parentClass.startsWith ("protected ")))
    {
        parentClass = "public " + parentClass;
    }

    for (int i = callbacks.size(); --i >= 0;)
    {
        CallbackMethod* const cm = callbacks.getUnchecked(i);

        if (cm->requiredParentClass == parentClass
             && cm->returnType == returnType
             && cm->prototype == prototype)
            return cm->content;
    }

    CallbackMethod* const cm = new CallbackMethod();
    callbacks.add (cm);

    cm->requiredParentClass = parentClass;
    cm->returnType = returnType;
    cm->prototype = prototype;
    cm->hasPrePostUserSections = hasPrePostUserSections;
    return cm->content;
}

void GeneratedCode::removeCallback (const String& returnType, const String& prototype)
{
    for (int i = callbacks.size(); --i >= 0;)
    {
        CallbackMethod* const cm = callbacks.getUnchecked(i);

        if (cm->returnType == returnType && cm->prototype == prototype)
            callbacks.remove (i);
    }
}

void GeneratedCode::addImageResourceLoader (const String& imageMemberName, const String& resourceName)
{
    privateMemberDeclarations
        << "Image " << imageMemberName << ";\n";

    if (resourceName.isNotEmpty())
        constructorCode << imageMemberName << " = ImageCache::getFromMemory ("
                        << resourceName << ", " << resourceName << "Size);\n";
}

StringArray GeneratedCode::getExtraParentClasses() const
{
    StringArray s;

    for (int i = 0; i < callbacks.size(); ++i)
    {
        CallbackMethod* const cm = callbacks.getUnchecked(i);
        s.add (cm->requiredParentClass);
    }

    return s;
}

String GeneratedCode::getCallbackDeclarations() const
{
    String s;

    for (int i = 0; i < callbacks.size(); ++i)
    {
        CallbackMethod* const cm = callbacks.getUnchecked(i);

        s << cm->returnType << " " << cm->prototype << ";\n";
    }

    return s;
}

String GeneratedCode::getCallbackDefinitions() const
{
    String s;

    for (int i = 0; i < callbacks.size(); ++i)
    {
        CallbackMethod* const cm = callbacks.getUnchecked(i);

        const String userCodeBlockName ("User"
            + CodeHelpers::makeValidIdentifier (cm->prototype.upToFirstOccurrenceOf ("(", false, false),
                                                true, true, false).trim());

        if (userCodeBlockName.isNotEmpty() && cm->hasPrePostUserSections)
        {
            s << cm->returnType << " " << className << "::" << cm->prototype
              << "\n{\n    //[" << userCodeBlockName << "_Pre]\n    //[/" << userCodeBlockName
              << "_Pre]\n\n    "
              << CodeHelpers::indent (cm->content.trim(), 4, false)
              << "\n\n    //[" << userCodeBlockName << "_Post]\n    //[/" << userCodeBlockName
              << "_Post]\n}\n\n";
        }
        else
        {
            s << cm->returnType << " " << className << "::" << cm->prototype
              << "\n{\n    "
              << CodeHelpers::indent (cm->content.trim(), 4, false)
              << "\n}\n\n";
        }
    }

    return s;
}

//==============================================================================
String GeneratedCode::getClassDeclaration() const
{
    StringArray parentClassLines;
    parentClassLines.addTokens (parentClasses, ",", StringRef());
    parentClassLines.addArray (getExtraParentClasses());

    parentClassLines = getCleanedStringArray (parentClassLines);

    if (parentClassLines.contains ("public Button", false))
        parentClassLines.removeString ("public Component", false);

    String r ("class ");
    r << className << "  : ";

    r += parentClassLines.joinIntoString (",\n" + String::repeatedString (" ", r.length()));

    return r;
}

String GeneratedCode::getInitialiserList() const
{
    StringArray inits (initialisers);

    if (parentClassInitialiser.isNotEmpty())
        inits.insert (0, parentClassInitialiser);

    inits = getCleanedStringArray (inits);

    String s;

    if (inits.size() == 0)
        return s;

    s << "    : ";

    for (int i = 0; i < inits.size(); ++i)
    {
        String init (inits[i]);

        while (init.endsWithChar (','))
            init = init.dropLastCharacters (1);

        s << init;

        if (i < inits.size() - 1)
            s << ",\n      ";
        else
            s << "\n";
    }

    return s;
}

static String getIncludeFileCode (StringArray files)
{
    String s;

    files = getCleanedStringArray (files);

    for (int i = 0; i < files.size(); ++i)
        s << "#include \"" << files[i] << "\"\n";

    return s;
}

bool GeneratedCode::shouldUseTransMacro() const noexcept
{
    return document->shouldUseTransMacro();
}

//==============================================================================
static void replaceTemplate (String& text, const String& itemName, const String& value)
{
    for (;;)
    {
        const int index = text.indexOf ("%%" + itemName + "%%");

        if (index < 0)
            break;

        int indentLevel = 0;

        for (int i = index; --i >= 0;)
        {
            if (text[i] == '\n')
                break;

            ++indentLevel;
        }

        text = text.replaceSection (index, itemName.length() + 4,
                                    CodeHelpers::indent (value, indentLevel, false));
    }
}

//==============================================================================
static bool getUserSection (const StringArray& lines, const String& tag, StringArray& resultLines)
{
    const int start = indexOfLineStartingWith (lines, "//[" + tag + "]", 0);

    if (start < 0)
        return false;

    const int end = indexOfLineStartingWith (lines, "//[/" + tag + "]", start + 1);

    for (int i = start + 1; i < end; ++i)
        resultLines.add (lines [i]);

    return true;
}

static void copyAcrossUserSections (String& dest, const String& src)
{
    StringArray srcLines, dstLines;
    srcLines.addLines (src);
    dstLines.addLines (dest);

    for (int i = 0; i < dstLines.size(); ++i)
    {
        if (dstLines[i].trimStart().startsWith ("//["))
        {
            String tag (dstLines[i].trimStart().substring (3));
            tag = tag.upToFirstOccurrenceOf ("]", false, false);

            jassert (! tag.startsWithChar ('/'));

            if (! tag.startsWithChar ('/'))
            {
                const int endLine = indexOfLineStartingWith (dstLines,
                                                             "//[/" + tag + "]",
                                                             i + 1);

                if (endLine > i)
                {
                    StringArray sourceLines;

                    if (getUserSection (srcLines, tag, sourceLines))
                    {
                        for (int j = endLine - i; --j > 0;)
                            dstLines.remove (i + 1);

                        for (int j = 0; j < sourceLines.size(); ++j)
                            dstLines.insert (++i, sourceLines [j].trimEnd());

                        ++i;
                    }
                    else
                    {
                        i = endLine;
                    }
                }
            }
        }

        dstLines.set (i, dstLines[i].trimEnd());
    }

    dest = dstLines.joinIntoString ("\n") + "\n";
}

//==============================================================================
void GeneratedCode::applyToCode (String& code,
                                 const String& fileNameRoot,
                                 const bool isForPreview,
                                 const String& oldFileWithUserData) const
{
    // header guard..
    String headerGuard ("__JUCE_HEADER_");
    headerGuard << String::toHexString ((className + "xx" + fileNameRoot).hashCode64()).toUpperCase() << "__";
    replaceTemplate (code, "headerGuard", headerGuard);

    replaceTemplate (code, "version", JUCEApplicationBase::getInstance()->getApplicationVersion());
    replaceTemplate (code, "creationTime", Time::getCurrentTime().toString (true, true, true));

    replaceTemplate (code, "className", className);
    replaceTemplate (code, "constructorParams", constructorParams);
    replaceTemplate (code, "initialisers", getInitialiserList());

    replaceTemplate (code, "classDeclaration", getClassDeclaration());
    replaceTemplate (code, "privateMemberDeclarations", privateMemberDeclarations);
    replaceTemplate (code, "publicMemberDeclarations", getCallbackDeclarations() + "\n" + publicMemberDeclarations);

    replaceTemplate (code, "methodDefinitions", getCallbackDefinitions());

    replaceTemplate (code, "includeFilesH", getIncludeFileCode (includeFilesH));
    replaceTemplate (code, "includeFilesCPP", getIncludeFileCode (includeFilesCPP));

    replaceTemplate (code, "constructor", constructorCode);
    replaceTemplate (code, "destructor", destructorCode);

    if (! isForPreview)
    {
        replaceTemplate (code, "metadata", jucerMetadata);
        replaceTemplate (code, "staticMemberDefinitions", staticMemberDefinitions);
    }
    else
    {
        replaceTemplate (code, "metadata", "  << Metadata isn't shown in the code preview >>\n");
        replaceTemplate (code, "staticMemberDefinitions", "// Static member declarations and resources would go here... (these aren't shown in the code preview)");
    }

    copyAcrossUserSections (code, oldFileWithUserData);
}
