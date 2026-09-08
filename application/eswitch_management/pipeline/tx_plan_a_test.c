#include "tx_plan_a.h"
#include <assert.h>
int main(void) {
  uint8_t vm[] = {0x7e,0x83,0xa5,0x77,0x11,6};
  assert(tx_plan_a_selected("100","7e:83:a5:77:11:06",100,vm));
  assert(!tx_plan_a_selected(NULL,"7e:83:a5:77:11:06",100,vm));
  assert(!tx_plan_a_selected("100",NULL,100,vm));
  assert(!tx_plan_a_selected("101","7e:83:a5:77:11:06",100,vm));
  assert(!tx_plan_a_selected("100","7e:83:a5:77:11:07",100,vm));
  assert(!tx_plan_a_selected("100garbage","7e:83:a5:77:11:06",100,vm));
  assert(!tx_plan_a_selected("0","7e:83:a5:77:11:06",0,vm));
  puts("PASS: Plan A selector, VS/VM isolation, missing and malformed selectors");
  return 0;
}
