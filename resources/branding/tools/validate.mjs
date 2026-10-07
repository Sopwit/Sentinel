import sharp from 'sharp';
import { readFile, writeFile, readdir } from 'node:fs/promises';
import { fileURLToPath } from 'node:url';
import { createHash } from 'node:crypto';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import assert from 'node:assert/strict';

const root=fileURLToPath(new URL('../',import.meta.url));
const geometry='M16 16H44V32L68 20L84 40L68 52L80 76L56 88L40 64L16 72V52L28 44L16 36Z';
const files=[];
async function walk(directory=''){for(const item of await readdir(path.join(root,directory),{withFileTypes:true})){const relative=path.posix.join(directory,item.name);if(item.isDirectory())await walk(relative);else files.push(relative);}}
await walk();
const svgFiles=files.filter(file=>file.endsWith('.svg'));
assert.equal(svgFiles.length,12,'unexpected SVG inventory');
execFileSync('python3',['-c',`import sys,xml.etree.ElementTree as ET
for filename in sys.argv[1:]:
 root=ET.parse(filename).getroot()
 assert root.tag=='{http://www.w3.org/2000/svg}svg',filename
 assert 'viewBox' in root.attrib,filename
forbidden={'image','filter','foreignObject','script','font','font-face','linearGradient','radialGradient'}
for filename in sys.argv[1:]:
 for element in ET.parse(filename).iter():
  assert element.tag.split('}')[-1] not in forbidden,filename
`,...svgFiles.map(file=>path.join(root,file))]);
for(const file of svgFiles){const source=await readFile(path.join(root,file),'utf8');assert(source.includes(`d="${geometry}"`),`${file}: master path absent`);assert(!/data:|@font-face|base64|\.woff|\.ttf/i.test(source),`${file}: embedded asset`);}
const symbol=await readFile(path.join(root,'symbol/sentinel-symbol.svg'),'utf8');assert(symbol.includes('viewBox="0 0 100 100"'));assert(symbol.includes('fill="currentColor"'));
const iconSvg=await readFile(path.join(root,'app-icon/master.svg'),'utf8');
const flatIcon=`<svg xmlns="http://www.w3.org/2000/svg" width="1024" height="1024" viewBox="0 0 160 160"><rect x="2" y="2" width="156" height="156" rx="34" fill="#151719"/><path transform="translate(8.5 5) scale(1.5)" fill="#ECEFEE" d="${geometry}"/></svg>\n`;
assert.equal(iconSvg,flatIcon,'Quiet Field must contain only the existing rounded Obsidian field and exact Porcelain FA-3');
assert.equal(await readFile(path.join(root,'app-icon/linux/scalable/apps/sentinel.svg'),'utf8'),flatIcon);
assert.equal(await readFile(path.join(root,'docs/github-avatar.svg'),'utf8'),flatIcon);
const nativeIconCache=new Map();
async function flatIconPixels(size){if(!nativeIconCache.has(size)){const source=flatIcon.replace('width="1024"',`width="${size}"`).replace('height="1024"',`height="${size}"`);nativeIconCache.set(size,await sharp(Buffer.from(source)).ensureAlpha().raw().toBuffer());}return nativeIconCache.get(size);}

const pngFiles=files.filter(file=>file.endsWith('.png'));
assert.equal(pngFiles.length,50,'unexpected PNG inventory');
let monochromeCount=0;
const rasterDetails=[];
for(const file of pngFiles){
  const match=file.match(/sentinel-(\d+)\.png$/)||file.match(/sentinel-tray-(\d+)-(?:black|white|template)(@2x)?\.png$/)||file.match(/favicon-(\d+)\.png$/)||file.match(/(\d+)x\d+\/apps\/sentinel\.png$/);
  assert(match,`${file}: dimension rule missing`);
  const expected=Number(match[1])*(match[2]==='@2x'?2:1);
  const {data,info}=await sharp(path.join(root,file)).ensureAlpha().raw().toBuffer({resolveWithObject:true});
  if(file.startsWith('app-icon/'))assert(data.equals(await flatIconPixels(expected)),`${file}: stale plane, non-flat background or altered symbol placement`);
  assert.equal(info.width,expected,file);assert.equal(info.height,expected,file);assert.equal(info.channels,4,file);
  let opaque=0,transparent=0;
  const monochrome=file.startsWith('tray/')||file.startsWith('favicon-');
  const foreground=file.includes('-white')?255:0;
  for(let offset=0;offset<data.length;offset+=4){
    if(data[offset+3]===0)transparent++;else{opaque++;if(monochrome){assert.equal(data[offset],foreground,file);assert.equal(data[offset+1],foreground,file);assert.equal(data[offset+2],foreground,file);}else{
      const channels=[data[offset],data[offset+1],data[offset+2]];
      const quantization=Math.ceil(255/data[offset+3]);
      assert(channels[0]>=21-quantization&&channels[0]<=236+quantization&&channels[1]>=23-quantization&&channels[1]<=239+quantization&&channels[2]>=25-quantization&&channels[2]<=238+quantization,`${file}: unexpected RGB range`);
    }}
  }
  assert(opaque>0&&transparent>0,`${file}: empty image or flattened background`);
  assert.equal(data[3],0,`${file}: top-left corner not transparent`);
  if(monochrome)monochromeCount++;
  rasterDetails.push({file,width:info.width,height:info.height,monochrome,transparentPixels:transparent});
}
for(const platform of ['windows','linux'])for(const size of platform==='windows'?[16,20,24,32]:[16,22,24,32]){
  const black=await sharp(path.join(root,`tray/${platform}/sentinel-tray-${size}-black.png`)).ensureAlpha().raw().toBuffer();
  const white=await sharp(path.join(root,`tray/${platform}/sentinel-tray-${size}-white.png`)).ensureAlpha().raw().toBuffer();
  for(let offset=3;offset<black.length;offset+=4)assert.equal(black[offset],white[offset],'light/dark alpha mismatch');
}
const pngSignature=Buffer.from([137,80,78,71,13,10,26,10]);
async function inspectPng(buffer,size){assert(buffer.subarray(0,8).equals(pngSignature));const metadata=await sharp(buffer).metadata();assert.equal(metadata.width,size);assert.equal(metadata.height,size);const pixels=await sharp(buffer).ensureAlpha().raw().toBuffer();assert(pixels.equals(await flatIconPixels(size)),'container contains stale plane-based icon');}
const ico=await readFile(path.join(root,'app-icon/windows/sentinel.ico'));
assert.equal(ico.readUInt16LE(0),0);assert.equal(ico.readUInt16LE(2),1);assert.equal(ico.readUInt16LE(4),8);
const windowsSizes=[16,20,24,32,48,64,128,256];let expectedOffset=6+8*16;
for(let index=0;index<8;index++){const start=6+index*16,size=ico[start]||256;assert.equal(size,windowsSizes[index]);assert.equal(ico[start+1]||256,size);assert.equal(ico.readUInt16LE(start+4),1);assert.equal(ico.readUInt16LE(start+6),32);const length=ico.readUInt32LE(start+8),offset=ico.readUInt32LE(start+12);assert.equal(offset,expectedOffset);assert(offset+length<=ico.length);await inspectPng(ico.subarray(offset,offset+length),size);expectedOffset+=length;}
assert.equal(expectedOffset,ico.length);
const icns=await readFile(path.join(root,'app-icon/macos/sentinel.icns'));assert.equal(icns.toString('ascii',0,4),'icns');assert.equal(icns.readUInt32BE(4),icns.length);
const types={icp4:16,icp5:32,icp6:64,ic07:128,ic08:256,ic09:512,ic10:1024,ic11:32,ic12:64,ic13:256,ic14:512};let offset=8;const found=[];
while(offset<icns.length){assert(offset+8<=icns.length);const type=icns.toString('ascii',offset,offset+4),length=icns.readUInt32BE(offset+4);assert(types[type],`unsupported ICNS entry ${type}`);assert(length>8&&offset+length<=icns.length);assert(!found.includes(type));await inspectPng(icns.subarray(offset+8,offset+length),types[type]);found.push(type);offset+=length;}
assert.equal(offset,icns.length);assert.equal(found.length,11);
const manifest=JSON.parse(await readFile(path.join(root,'manifest.json'),'utf8'));assert.equal(manifest.geometry,geometry);
assert.equal(manifest.generatedAssetCount,64);
assert.equal(manifest.authoritativePath,geometry);
assert.equal(manifest.symbol,'FA-3');
assert.equal(manifest.appIcon,'Quiet Field');
assert.equal(manifest.appIconComposition.symbolTransform,'translate(8.5 5) scale(1.5)');
assert.equal(manifest.appIconComposition.symbolFill,'#ECEFEE');
assert.equal(manifest.appIconComposition.container.fill,'#151719');
assert.equal(manifest.appIconComposition.retired.length,4);
assert.equal(manifest.assetStatus,'production-candidate');
assert.equal(manifest.identityStatus,'PROVISIONAL LOCK');
assert(!Number.isNaN(Date.parse(manifest.generationDate)));
assert.equal(manifest.deferredValidationItems.length,5);
const expectedFiles=new Set([...manifest.files.map(entry=>entry.path),'manifest.json','validation-report.json']);
assert.equal(expectedFiles.size,manifest.package.brandingFileCount);
assert(files.every(file=>expectedFiles.has(file)),'unexpected branding file');
assert([...expectedFiles].every(file=>file==='validation-report.json'||files.includes(file)),'missing branding file');
for(const entry of manifest.files){const buffer=await readFile(path.join(root,entry.path));assert.equal(buffer.length,entry.bytes,entry.path);assert.equal(createHash('sha256').update(buffer).digest('hex'),entry.sha256,entry.path);}
function luminance(hex){const channels=hex.match(/[a-f0-9]{2}/gi).map(channel=>parseInt(channel,16)/255).map(value=>value<=.04045?value/12.92:((value+.055)/1.055)**2.4);return channels[0]*.2126+channels[1]*.7152+channels[2]*.0722;}
const contrast=[['Black / white','#000000','#FFFFFF'],['Porcelain / Obsidian','#ECEFEE','#151719'],['Porcelain / Graphite','#ECEFEE','#303438']].map(([pair,foreground,background])=>{const values=[luminance(foreground),luminance(background)].sort((left,right)=>right-left);const ratio=(values[0]+.05)/(values[1]+.05);assert(ratio>=4.5,`${pair}: insufficient contrast`);return {pair,ratio:Number(ratio.toFixed(2))};});
const report={verdict:'PASS — FILE/GEOMETRY VALIDATION ONLY',svgCount:svgFiles.length,pngCount:pngFiles.length,monochromePngCount:monochromeCount,icoRepresentations:windowsSizes,icnsRepresentations:found,contrast,checks:['XML parsing','exact FA-3 path','flat Quiet Field: only Obsidian + Porcelain FA-3','all app-icon PNGs and container entries match the exact two-shape reference; no secondary plane','no embedded fonts/raster/filter/gradient','raster dimensions and transparent corners','monochrome tray/favicons','matching light/dark alpha','foreground/background contrast','ICO directory/offsets/decoded PNG entries','ICNS header/chunks/decoded PNG entries','manifest SHA-256 inventory'],limitations:['Native OS shell integration not executed.','Small-size recognition and external similarity risk remain unresolved.','Live-text SVG typography requires Inter/IBM Plex Mono or uses fallbacks.','No legal or trademark clearance.'],rasters:rasterDetails};
await writeFile(path.join(root,'validation-report.json'),JSON.stringify(report,null,2)+'\n');
manifest.validation={status:'PASS',scope:'asset file formats and frozen geometry only',report:'validation-report.json',legalClearance:'NOT CLAIMED',externalRecognition:'DEFERRED / EVIDENCE WAIVED',externalSimilarity:'DEFERRED / EVIDENCE WAIVED',nativeOsTray:'PENDING REAL OS VALIDATION'};
await writeFile(path.join(root,'manifest.json'),JSON.stringify(manifest,null,2)+'\n');
console.log(JSON.stringify({...report,rasters:undefined},null,2));
if(process.argv.includes('--package')){
  const output=fileURLToPath(new URL('../../../public/sentinel-brand-production-candidate.zip',import.meta.url));
  const result=execFileSync('python3',['-c',`import sys,os,json,zipfile,hashlib
root,output=sys.argv[1:]
with open(os.path.join(root,'manifest.json'),encoding='utf-8') as stream:
 manifest=json.load(stream)
entries={}
for item in manifest['files']:
 relative=item['path']
 assert not relative.startswith('/') and '..' not in relative.split('/'),relative
 entries['resources/branding/'+relative]=os.path.join(root,relative)
for relative in ['manifest.json','validation-report.json']:
 entries['resources/branding/'+relative]=os.path.join(root,relative)
for alias in manifest['package']['rootAliases']:
 entries[alias['path']]=os.path.join(root,alias['path'])
 assert hashlib.sha256(open(entries[alias['path']],'rb').read()).hexdigest()==alias['sha256']
assert len(entries)==manifest['package']['archiveFileCount']
os.makedirs(os.path.dirname(output),exist_ok=True)
with zipfile.ZipFile(output,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as archive:
 for member,source in sorted(entries.items()):
  archive.write(source,member)
with zipfile.ZipFile(output,'r') as archive:
 members=archive.namelist()
 assert len(members)==len(set(members))==len(entries)
 assert set(members)==set(entries)
 assert archive.testzip() is None
 for member in members:
  assert archive.read(member)==open(entries[member],'rb').read(),member
  assert not any(part in {'node_modules','dist','build','validation-review','.git'} for part in member.split('/')),member
 for item in manifest['files']:
  assert hashlib.sha256(archive.read('resources/branding/'+item['path'])).hexdigest()==item['sha256'],item['path']
print(json.dumps({'archive':output,'inventory':'PASS','files':len(entries),'brandingFiles':manifest['package']['brandingFileCount'],'rootFavicons':len(manifest['package']['rootAliases']),'sha256':hashlib.sha256(open(output,'rb').read()).hexdigest(),'bytes':os.path.getsize(output)}))
`,root,output],{encoding:'utf8'});
  console.log(result.trim());
}
