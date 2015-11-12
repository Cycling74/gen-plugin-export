#!/usr/bin/env python

# Copyright (c) 2015 Cycling '74

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

import os
import sys
import argparse
import time
from xml.dom import minidom

def which(program):
	import os
	def is_exe(fpath):
		return os.path.isfile(fpath) and os.access(fpath, os.X_OK)

	fpath, fname = os.path.split(program)
	if fpath:
		if is_exe(program):
			return program
	else:
		for path in os.environ["PATH"].split(os.pathsep):
			path = path.strip('"')
			exe_file = os.path.join(path, program)
			if is_exe(exe_file):
				return exe_file

	return None

currentdir = os.path.dirname(os.path.realpath(__file__))

parser = argparse.ArgumentParser()
parser.add_argument("--type", type=str, default="VST", help='Plugin/Application type')
parser.add_argument("--name", type=str, default="", help='Plugin/Application name')
parser.add_argument("--channelconf", type=str, default="{1,1}, {2,2}", help='Plugin channel configuration')
parser.add_argument("--configuration", type=str, default="Debug", help='Build configuration (Debug/Release)')

args = parser.parse_args()

if args.name == "":
	args.name = "C74-Gen-" + args.type + "Plugin"

# find out which jucer file we need
if args.type == "iOS":
	jucername = "C74-Gen-Application.jucer"
else:
	jucername = "C74-Gen-" + args.type + "Plugin.jucer"

# now edit it to adapt it to our command line options

xmldoc = minidom.parse(currentdir + "/Introjucer/" + jucername)
juceproject = xmldoc.getElementsByTagName("JUCERPROJECT")

juceproject[0].attributes["name"] = args.name
juceproject[0].attributes["bundleIdentifier"] = "com.cycling74." + args.name
juceproject[0].attributes["pluginName"] = args.name
juceproject[0].attributes["pluginAUExportPrefix"] = args.name + "AU"
juceproject[0].attributes["aaxIdentifier"] = "com.cycling74." + args.name

juceproject[0].attributes["pluginChannelConfigs"] = args.channelconf

maingroup = juceproject[0].getElementsByTagName("MAINGROUP")
maingroup[0].attributes["name"] = args.name

exportformats = juceproject[0].getElementsByTagName("EXPORTFORMATS")

xcode_mac = exportformats[0].getElementsByTagName("XCODE_MAC")
if xcode_mac:
	conf = xcode_mac[0].getElementsByTagName("CONFIGURATION")
	conf[0].attributes["targetName"] = args.name

xcode_iphone = exportformats[0].getElementsByTagName("XCODE_IPHONE")
if xcode_iphone:
	conf = xcode_iphone[0].getElementsByTagName("CONFIGURATION")
	conf[0].attributes["targetName"] = args.name

vs2013 = exportformats[0].getElementsByTagName("VS2013")
if vs2013:
	conf = vs2013[0].getElementsByTagName("CONFIGURATION")
	conf[0].attributes["targetName"] = args.name

writetmpjucer = True

tmpprojname = currentdir + "/Introjucer/tmp-" + jucername
tmpprojpresent = os.path.isfile(tmpprojname)

if tmpprojpresent:
	tmpxmldoc = minidom.parse(tmpprojname)
	if tmpxmldoc:
		tmpjuceproj = tmpxmldoc.getElementsByTagName("JUCERPROJECT")
		print("INTROJUCER PROJECT NAME: " + tmpjuceproj[0].attributes["name"].value)
		print("INTROJUCER PROJECT CHANNELCONFIG: " + tmpjuceproj[0].attributes["pluginChannelConfigs"].value)
		if tmpjuceproj and (tmpjuceproj[0].attributes["name"].value == args.name) and (tmpjuceproj[0].attributes["pluginChannelConfigs"].value == args.channelconf):
			writetmpjucer = False
else:
	print("NO TEMP INTROJUCER PROJECT FOUND")

# write out tmp.jucer
if writetmpjucer:
	print("WRITING TEMP INTROJUCER PROJECT")
	fh = open(tmpprojname,"w")
	xmldoc.writexml(fh)
	fh.close()
else:
	print("USING CACHED INTROJUCER PROJECT")

print(sys.platform)

# now re-create the Juce project an build it

if sys.platform.startswith("darwin"):
	if writetmpjucer:
		cmd = "open -n "+ currentdir + "/Introjucer/Introjucer.app  --args --resave \"" + tmpprojname + "\""
		print(cmd)
		os.system(cmd)
		time.sleep(2)

	xcodebuildcmd = which("xcodebuild")
	if xcodebuildcmd == "":
		sys.exit("-> Could not locate 'xcodebuild', which is necessary to build on OSX. Make sure that Xcode is installed.")

	project = ""
	open = False

	if args.type == "iOS":
		open = True
		project = currentdir + "/App-Builds/iOS/"+ args.name + ".xcodeproj"
	else:
		project = currentdir + "/" + args.type + "-Builds/MacOSX/" + args.name + ".xcodeproj"

	if project != "":

		if open:
			os.system("open /Applications/Xcode.app " + project)
		else:
			cmd = xcodebuildcmd + " -project " + project + " -configuration " + args.configuration
			print(cmd)
			os.system(cmd)

elif sys.platform.startswith("win"):
	tmpprojname = tmpprojname.replace("/", "\\" )
	cmd = '"' + currentdir + '\\Introjucer\\The Introjucer.exe" --resave ' + tmpprojname
	print(cmd)
	os.system(cmd)

	time.sleep(2)

	project = ""

	if args.type == "VST":
		project = currentdir + "/VST-Builds/VisualStudio2013/" + args.name + ".vcxproj"
	elif args.type == "VST3":
		project = currentdir + "/VST3-Builds/VisualStudio2013/" + args.name + ".vcxproj"

	project = project.replace("/", "\\" )
	print("trying to open project... " + project)

	if project != "":
		callstr = "tasklist /nh /fi \"imagename eq WDExpress.exe\" /fi \"WINDOWTITLE eq " + args.name + "*\" | find /i \"WDExpress.exe\" > nul || tasklist /nh /fi \"imagename eq devenv.exe\" /fi \"WINDOWTITLE eq " + args.name + "*\" | find /i \"devenv.exe\" > nul || (cmd.exe /c \"" + project + "\")"

		print(callstr)
		os.system(callstr)
