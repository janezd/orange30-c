import urllib.request, urllib.parse, urllib.error, sys, os, md5

files = "orange", "corn", "statc", "orangeom", "orangene", "_orngCRS"
baseurl = "http://www.ailab.si/orange/download/binaries/%i%i/" % sys.version_info[:2]
fleurl = baseurl + "%s.pyd"

op = [x for x in sys.path if x[-7:].lower() in ["\\orange", "/orange"]]
if not op:
	print("Orange is not found on the Python's path")

print("Downloading to %s (for Python %i.%i)" % (op[0], sys.version_info[0], sys.version_info[1]))
os.chdir(op[0])

def rep(blk_cnt, blk_size, tot_size):
	print("\rDownloading %s: %i of %i" % (fle, min(tot_size, blk_cnt*blk_size), tot_size), end=' ')

repository_stamps = dict([tuple(x.split()) for x in urllib.request.urlopen(baseurl + "stamps_pyd.txt") if x.strip()])

for fle in files:
    if os.path.exists(fle+".pyd") and repository_stamps[fle+".pyd"] == md5.md5(open(fle+".pyd", "rb").read()).hexdigest().upper():
        print("\nSkipping %s" % fle, end=' ')
    else:
        print("\nDownloading %s" % fle, end=' ')
        urllib.request.urlretrieve(fleurl % fle, fle+".temp", rep)
        if os.path.exists(fle+".pyd"):
            os.remove(fle+".pyd")
        os.rename(fle+".temp", fle+".pyd")
