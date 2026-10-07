import sharp from 'sharp';
import { mkdir, writeFile, readFile, readdir } from 'node:fs/promises';
import { createHash } from 'node:crypto';
import { fileURLToPath } from 'node:url';
import path from 'node:path';

const root = fileURLToPath(new URL('../', import.meta.url));
const geometry = 'M16 16H44V32L68 20L84 40L68 52L80 76L56 88L40 64L16 72V52L28 44L16 36Z';
const palette = { Obsidian: '#151719', Graphite: '#303438', Fog: '#929A9F', Porcelain: '#ECEFEE', Glacier: '#A9CAD3' };
const macSizes = [16,32,64,128,256,512,1024];
const windowsSizes = [16,20,24,32,48,64,128,256];
const linuxSizes = [16,22,24,32,48,64,128,256,512];
const generated = [];

async function save(relative, contents) {
  const destination = path.join(root, relative);
  await mkdir(path.dirname(destination), { recursive: true });
  const buffer=Buffer.isBuffer(contents)?contents:Buffer.from(contents);
  let unchanged=false;
  try{unchanged=(await readFile(destination)).equals(buffer);}catch(error){if(error.code!=='ENOENT')throw error;}
  if(!unchanged)await writeFile(destination,buffer);
  generated.push(relative);
}
function svg(width, height, viewBox, content) {
  return `<svg xmlns="http://www.w3.org/2000/svg" width="${width}" height="${height}" viewBox="${viewBox}">${content}</svg>\n`;
}
function mark(fill = 'currentColor', transform = '') {
  return `<path${transform ? ` transform="${transform}"` : ''} fill="${fill}" d="${geometry}"/>`;
}
const symbol = svg(100,100,'0 0 100 100',mark());
const black = svg(100,100,'0 0 100 100',mark('#000000'));
const white = svg(100,100,'0 0 100 100',mark('#FFFFFF'));
const icon = svg(1024,1024,'0 0 160 160',`<rect x="2" y="2" width="156" height="156" rx="34" fill="${palette.Obsidian}"/>${mark(palette.Porcelain,'translate(8.5 5) scale(1.5)')}`);
async function raster(relative, source, size) {
  const native = source.replace(/width="[^"]+"/, `width="${size}"`).replace(/height="[^"]+"/, `height="${size}"`);
  const buffer = await sharp(Buffer.from(native)).ensureAlpha().png({ compressionLevel: 9 }).toBuffer();
  await save(relative, buffer);
  return buffer;
}

await save('symbol/sentinel-symbol.svg',symbol);
await save('symbol/sentinel-symbol-black.svg',black);
await save('symbol/sentinel-symbol-white.svg',white);
await save('app-icon/master.svg',icon);
const macRasters = new Map();
for (const size of macSizes) macRasters.set(size,await raster(`app-icon/macos/sentinel-${size}.png`,icon,size));
const icnsRepresentations = [['icp4',16],['icp5',32],['icp6',64],['ic07',128],['ic08',256],['ic09',512],['ic10',1024],['ic11',32],['ic12',64],['ic13',256],['ic14',512]];
const icnsChunks = icnsRepresentations.map(([type,size]) => {
  const png = macRasters.get(size);
  const header = Buffer.alloc(8);
  header.write(type,0,4,'ascii');
  header.writeUInt32BE(png.length+8,4);
  return Buffer.concat([header,png]);
});
const icnsHeader = Buffer.alloc(8);
icnsHeader.write('icns',0,4,'ascii');
icnsHeader.writeUInt32BE(8+icnsChunks.reduce((sum,chunk)=>sum+chunk.length,0),4);
await save('app-icon/macos/sentinel.icns',Buffer.concat([icnsHeader,...icnsChunks]));

const windowsRasters = [];
for (const size of windowsSizes) windowsRasters.push(await raster(`app-icon/windows/sentinel-${size}.png`,icon,size));
const icoHeader = Buffer.alloc(6);
icoHeader.writeUInt16LE(1,2);
icoHeader.writeUInt16LE(windowsSizes.length,4);
let offset = 6+16*windowsSizes.length;
const icoEntries = windowsSizes.map((size,index)=>{
  const entry = Buffer.alloc(16);
  entry[0]=size===256?0:size;
  entry[1]=size===256?0:size;
  entry.writeUInt16LE(1,4);
  entry.writeUInt16LE(32,6);
  entry.writeUInt32LE(windowsRasters[index].length,8);
  entry.writeUInt32LE(offset,12);
  offset+=windowsRasters[index].length;
  return entry;
});
await save('app-icon/windows/sentinel.ico',Buffer.concat([icoHeader,...icoEntries,...windowsRasters]));
await save('app-icon/linux/scalable/apps/sentinel.svg',icon);
for (const size of linuxSizes) await raster(`app-icon/linux/${size}x${size}/apps/sentinel.png`,icon,size);

await save('tray/sentinel-tray.svg',symbol);
for (const size of [16,18,20,22]) {
  await raster(`tray/macos/sentinel-tray-${size}-template.png`,black,size);
  await raster(`tray/macos/sentinel-tray-${size}-template@2x.png`,black,size*2);
}
for (const [platform,sizes] of [['windows',[16,20,24,32]],['linux',[16,22,24,32]]]) {
  for (const size of sizes) {
    await raster(`tray/${platform}/sentinel-tray-${size}-black.png`,black,size);
    await raster(`tray/${platform}/sentinel-tray-${size}-white.png`,white,size);
  }
}
await save('tray/linux/sentinel-tray-symbolic.svg',symbol);
await save('wordmark/sentinel-lockup.svg',svg(290,100,'0 0 290 100',`${mark()}<text x="96" y="65" fill="currentColor" font-family="Inter, sans-serif" font-size="48" font-weight="500" letter-spacing="-1.4">sentinel.</text>`));
await save('docs/github-avatar.svg',icon);
await save('docs/github-social-preview.svg',svg(1280,640,'0 0 1280 640',`<rect width="1280" height="640" fill="${palette.Obsidian}"/><path d="M1110 0H1280V640H1110Z" fill="${palette.Graphite}"/>${mark(palette.Porcelain,'translate(86 78) scale(1.3)')}<text x="104" y="348" fill="${palette.Porcelain}" font-family="Inter, sans-serif" font-size="90" font-weight="500" letter-spacing="-3">sentinel.</text><text x="108" y="416" fill="${palette.Fog}" font-family="Inter, sans-serif" font-size="30" font-weight="400">Desktop intelligence. Quietly capable.</text><text x="108" y="554" fill="${palette.Glacier}" font-family="IBM Plex Mono, monospace" font-size="16" letter-spacing="2">PRECISE BY DESIGN</text>`));
await save('docs/readme-header.svg',svg(1600,480,'0 0 1600 480',`<rect width="1600" height="480" fill="${palette.Obsidian}"/><path d="M1460 0H1600V480H1460Z" fill="${palette.Graphite}"/>${mark(palette.Porcelain,'translate(96 110) scale(2.2)')}<text x="352" y="230" fill="${palette.Porcelain}" font-family="Inter, sans-serif" font-size="94" font-weight="500" letter-spacing="-3">sentinel.</text><text x="358" y="298" fill="${palette.Fog}" font-family="Inter, sans-serif" font-size="28">Desktop intelligence. Quietly capable.</text>`));
await save('favicon.svg',svg(100,100,'0 0 100 100',`<style>path{fill:${palette.Obsidian}}@media(prefers-color-scheme:dark){path{fill:${palette.Porcelain}}}</style>${mark()}`));
await raster('favicon-16.png',black,16);
await raster('favicon-32.png',black,32);

const files = [];
async function inventory(directory='') {
  for (const entry of await readdir(path.join(root,directory),{withFileTypes:true})) {
    const relative=path.posix.join(directory,entry.name);
    if(entry.isDirectory())await inventory(relative);
    else if(relative!=='manifest.json'&&relative!=='validation-report.json'){
      const contents=await readFile(path.join(root,relative));
      files.push({path:relative,bytes:contents.length,sha256:createHash('sha256').update(contents).digest('hex')});
    }
  }
}
await inventory();
const rootAliases=files.filter(entry=>['favicon.svg','favicon-16.png','favicon-32.png'].includes(entry.path)).map(entry=>({path:entry.path,source:`resources/branding/${entry.path}`,bytes:entry.bytes,sha256:entry.sha256}));
await save('manifest.json',JSON.stringify({assetVersion:'0.2.0-production-candidate',assetStatus:'production-candidate',identityStatus:'PROVISIONAL LOCK',status:'PROVISIONAL LOCK / EXTERNAL EVIDENCE WAIVED',symbol:'FA-3',geometry,authoritativePath:geometry,palette,appIcon:'Quiet Field',appIconComposition:{definition:'A solid Obsidian application field containing the Porcelain FA-3 symbol. No secondary plane or structural accent is used.',viewBox:'0 0 160 160',container:{x:2,y:2,width:156,height:156,rx:34,fill:'#151719'},symbolFill:'#ECEFEE',symbolTransform:'translate(8.5 5) scale(1.5)',retired:['original full-height side plane','Narrow Plane','Inset Plane','Minimal Edge']},generationDate:new Date().toISOString(),validation:{status:'PENDING',scope:'asset file formats and frozen geometry only',legalClearance:'NOT CLAIMED'},deferredValidationItems:['external blind-recognition review','external similarity/trademark review','native macOS menu-bar validation','native Windows tray validation','native Linux tray validation'],renderer:{sharp:sharp.versions.sharp,librsvg:sharp.versions.rsvg},evidenceWaiver:'External blind-recognition and similarity clearance was deferred. These assets are production-candidate identity assets, not trademark/legal clearance.',generatedAssetCount:generated.length,package:{filename:'sentinel-brand-production-candidate.zip',brandingFileCount:files.length+2,archiveFileCount:files.length+2+rootAliases.length,rootAliases},files:files.sort((left,right)=>left.path.localeCompare(right.path))},null,2)+'\n');
console.log(`Exported ${generated.length-1} identity files plus manifest into resources/branding/.`);
