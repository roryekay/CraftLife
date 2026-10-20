import {readFileSync} from 'node:fs';
const catalog=JSON.parse(readFileSync(new URL('./moments.json',import.meta.url),'utf8'));
export function destination(game,index){
  if(!Object.hasOwn(catalog,game)||!['halflife','halflife2'].includes(game)||!Number.isInteger(index)||index<0||index>=catalog[game].length)throw Error('Invalid travel destination');
  const entry=catalog[game][index];
  if(!/^[a-z0-9_]{1,63}$/.test(entry.map)||!Number.isInteger(entry.encounter)||entry.encounter<0||entry.encounter>9)throw Error('Invalid catalog entry');
  return entry;
}
