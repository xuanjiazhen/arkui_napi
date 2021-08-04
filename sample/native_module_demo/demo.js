const demoSo = requireInternal('demo');

console.info('before new');
const demoClass = new demoSo.DemoClass();
console.info('after new');

// 可以使用其它子系统的 js API
const ability = requireNapi('ability');

export default {
   add: demoSo.add,
   sub: demoClass.sub,
   FeadConstant: {
       TYPE_VALUE_0: 0,
       TYPE_VALUE_1: demoClass.THREE,
       TYPE_FROM_ABILITY: ability.DemoClass.FOUR
   }
}
