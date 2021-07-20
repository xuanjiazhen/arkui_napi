const demoSo = requireInternal('demo');

// 可以使用其它子系统的 js API
const ability = requireNapi('ability');

export default {
   add: demoSo.add,
   FeadConstant: {
       TYPE_VALUE_0: 0,
       TYPE_VALUE_1: 1,
       TYPE_FROM_ABILITY: ability.DemoClass.FOUR
   }
}