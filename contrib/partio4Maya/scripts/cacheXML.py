import re
import os
import math
import glob
import subprocess

InMaya = False
try:
   import maya.cmds as cmds
   import maya.OpenMaya as OpenMaya
   InMaya = True
except:
   pass

import xml.etree.ElementTree as ET
from xml.dom import minidom


def AsTicks(frame, fps=None):
   global InMaya
   
   if InMaya:
      t = OpenMaya.MTime(frame, OpenMaya.MTime.uiUnit())
      ticks = t.asUnits(OpenMaya.MTime.k6000FPS)
      return int(math.floor(ticks))
   else:
      if fps is None:
         fps = 24.0
      tickduration = fps / 6000.0
      ticks = frame / tickduration
      return int(math.floor(ticks))


def ListAttributes(cachePath):
   global InMaya
   
   rv = {}
   if InMaya:
      for attrib in cmds.partioImport(cachePath, query=1, listAttributes=1):
         rv[attrib] = cmds.partioImport(cachePath, query=1, attributeType=attrib)[0]
   else:
      cmd = "partinfo \"%s\"" % cachePath
      p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
      out, _ = p.communicate()
      attribexp = re.compile(r"(INT|FLOAT|VECTOR|INDEXEDSTR)\s+(\d+)\s+([^\s]+)\s*$")
      inattribs = False
      for l in out.split("\n"):
         m = attribexp.match(l.strip())
         if inattribs and not m:
            break
         elif m:
            inattribs = True
            atype = m.group(1)
            acount = int(m.group(2))
            aname = m.group(3)
            rv[aname] = atype
   
   return rv


def Create(cachePath, channelPrefix=None, xmlPath=None, frameRange=None, frameStep=1.0, frameRate=None, verbose=False):
   global InMaya
   
   frameexp = re.compile(r"\.(\d+(\.\d+)?)\.([^.]+)$")
   
   if frameRange is None:
      if InMaya:
         startframe = cmds.playbackOptions(query=1, animationStartTime=1)
         endframe = cmds.playbackOptions(query=1, animationEndTime=1)
      else:
         bn = os.path.basename(cachePath)
         m = frameexp.search(bn)
         if m:
            pattern = os.path.join(os.path.dirname(cachePath), bn[:m.start()]+".*."+m.group(3))
            samples = glob.glob(pattern)
            samples.sort()
            startframe = float(frameexp.search(samples[0]).group(1))
            endframe = float(frameexp.search(samples[-1]).group(1))
         else:
            startframe = 1.0
            endframe = startframe
      if verbose:
         print("Use frame range %f - %f" % (startframe, endframe))
   else:
      startframe, endframe = frameRange
   
   startticks = AsTicks(startframe, frameRate)
   endticks = AsTicks(endframe, frameRate)
   stepticks = AsTicks(frameStep, frameRate)
   
   if xmlPath is None:
      xmlPath = frameexp.sub(".xml", cachePath)
      if verbose:
         print("Output XML to \"%s\"" % xmlPath)
   
   xmldoc = ET.ElementTree()
   xmlroot = ET.Element("Autodesk_Cache_File")
   xmldoc._setroot(xmlroot)
   
   xmlnode = ET.Element("cacheType")
   xmlnode.set("Type", "OneFile")
   xmlnode.set("Format", "partio-gto")
   xmlroot.append(xmlnode)
   
   xmlnode = ET.Element("time")
   xmlnode.set("Range", "%d-%d" % (startticks, endticks))
   xmlroot.append(xmlnode)
   
   xmlnode = ET.Element("cacheTimePerFrame")
   xmlnode.set("TimePerFrame", str(AsTicks(1.0, frameRate)))
   xmlroot.append(xmlnode)
   
   xmlnode = ET.Element("cacheVersion")
   xmlnode.set("Version", "2.0")
   xmlroot.append(xmlnode)
   
   xmlnode = ET.Element("extra")
   xmlnode.text = "PartIO cache .gto"
   xmlroot.append(xmlnode)
   
   channels = ET.Element("Channels")
   
   # Could also use partinfo and partattr
   curchannel = 0
   prefix = ("" if channelPrefix is None else "%s_" % channelPrefix)
   
   attribs = ListAttributes(cachePath)
   for attrib, typ in attribs.iteritems():
      ct = None
      
      if typ in ["INT", "FLOAT"]:
         # Int32Array for INT?
         ct = "DoubleArray"
      elif typ == "VECTOR":
         ct = "FloatVectorArray"
      else:
         if verbose:
            print("Ignore channel \"%s\" of type %s" % (attrib, typ))
      
      if ct:
         channel = ET.Element("channel%d" % curchannel)
         channel.set("ChannelType", ct)
         channel.set("ChannelName", prefix+attrib)
         channel.set("ChannelInterpretation", attrib)
         channel.set("SamplingType", "Irregular")
         channel.set("SamplingRate", str(stepticks))
         channel.set("StartTime", str(startticks))
         channel.set("EndTime", str(endticks))
         channels.append(channel)
         curchannel += 1
   
   # Add "count" channel
   channel = ET.Element("channel%d" % curchannel)
   channel.set("ChannelName", prefix+"count")
   channel.set("ChannelType", "DoubleArray")
   channel.set("ChannelInterpretation", "count")
   channel.set("SamplingType", "Irregular")
   channel.set("SamplingRate", str(stepticks))
   channel.set("StartTime", str(startticks))
   channel.set("EndTime", str(endticks))
   channels.append(channel)
   
   xmlroot.append(channels)
   
   # do not write using xmldoc.write -> bad indentation
   xmlfile = open(xmlPath, "w")
   xmlfile.write(minidom.parseString(ET.tostring(xmlroot)).toprettyxml())
   xmlfile.close()
   
   return xmlPath


if __name__ == "__main__":
   import sys
   
   xmlpath = None
   framerange = None
   framestep = 1.0
   framerate = None
   cachepath = None
   verbose = False
   prefix = None
   
   args = sys.argv[1:]
   i = 0
   n = len(args)
   while i < n:
      arg = args[i]
      if arg in ["-xp", "--xml-path"]:
         i += 1
         if i >= n:
            raise Exception("-xp/--xml-path flag requires 1 argument")
         xmlpath = args[i]
      elif arg in ["-fps", "--frame-rate"]:
         i += 1
         if i >= n:
            raise Exception("-fps/--frame-rate flag requires 1 argument")
         framerate = float(args[i])
      elif arg in ["-fr", "--frame-range"]:
         i += 2
         if i >= n:
            raise Exception("-fr/--frame-range flag requires 2 arguments")
         framerange = (float(args[i-1]), float(args[i]))
         pass
      elif arg in ["-fs", "--frame-step"]:
         i += 1
         if i >= n:
            raise Exception("-fs/--frame-step flag requires 1 argument")
         framestep = float(args[i])
      elif arg in ["-cp", "--channel-prefix"]:
         i += 1
         if i >= n:
            raise Exception("-cp/--channel-prefix flag requires 1 argument")
         prefix = args[i]
      elif arg in ["-v", "--verbose"]:
         verbose = True
      else:
         if cachepath is None:
            cachepath = arg
         else:
            raise Exception("command only accepts 1 argument") 
      i += 1
   
   Create(cachepath, xmlPath=xmlpath, channelPrefix=prefix, frameRange=framerange, frameStep=framestep, frameRate=framerate, verbose=verbose)
   sys.exit(0)


