# -*- coding: latin-1 -*-

import ftplib
from getpass import getpass
from VersionInfo import CalcVersionInfo
import os
import sys
import traceback
import datetime
import subprocess

try:
  # Test that Graph can be started without some obscure dll files
  os.system("Graph.exe /regserver")
  VersionInfo = CalcVersionInfo("Graph.exe")

  # Sign Graph.exe
  subprocess.check_call(['signtool.exe',  'sign', '/sha1', '2b23c09a501a21ef4b37d33894bbf8bcc59cdc76', '/tr', 'http://time.certum.pl', '/fd', 'sha256', '/d', 'Graph ' + VersionInfo, 'Graph.exe'])

  # Compile SetupGraphBeta-4.5.x.exe
  print("Compiling...")
  subprocess.check_call(["c:\\program files (x86)\\Inno Setup 5\\iscc.exe", "/Q", "Graph.iss"])

  FileName = "SetupGraphBeta-" + VersionInfo + ".exe"

  # Sign SetupGraphBeta-x.x.x.exe
#  subprocess.check_call(['signtool.exe',  'sign', '/sha1', '2b23c09a501a21ef4b37d33894bbf8bcc59cdc76', '/tr', 'http://time.certum.pl', '/fd', 'sha256', '/d', 'Graph ' + VersionInfo, FileName])

  #Creating GraphBeta.inf
  print("Writing GraphBeta.inf ...")
  File = open("GraphBeta.inf", "w")
  File.write("[Graph]\n")
  File.write("Major = " + VersionInfo[0] + "\n")
  File.write("Minor = " + VersionInfo[2] + "\n")
  File.write("Release = " + VersionInfo[4] + "\n")
  File.write("Build = " + VersionInfo[6:] + "\n")
  File.write("Date = " + datetime.date.today().strftime("%d-%m-%Y\n"))
  File.write("DownloadFile = http://www.padowan.dk/bin/" + FileName + '\n')
  File.write("DownloadPage = http://www.padowan.dk/beta\n")

  # Upload SetupGraphBeta.exe to the server
  ftp = ftplib.FTP('ftp.padowan.dk')   # connect to host, default port
  while True:
    try:
      Password = getpass()
      ftp.login('padowan.dk', Password)
      break
    except ftplib.error_perm as E:
        print(E.args)
  ftp.cwd('bin')
  print("Uploading", FileName, "...")
  File = open(FileName, 'rb')
  ftp.storbinary('STOR ' + FileName, File)
  if ftp.size(FileName) != os.stat(FileName).st_size:
    raise Exception("Wrong file size on server")

  print("Uploading GraphBeta.inf ...")
  ftp.cwd('../graph')
  File = open("GraphBeta.inf", 'rb')
  ftp.storbinary('STOR GraphBeta.inf', File)

  ftp.quit()
  print("Upload complete!")
except Exception:
  traceback.print_exception(sys.exc_info()[0], sys.exc_info()[1], sys.exc_info()[2])
