#ifndef _FH_OSAL_CLK_PIN_H
#define _FH_OSAL_CLK_PIN_H

void *fh_osal_devm_pinctrl_get(void *dev);
void fh_osal_devm_pinctrl_put(void *p);
void *fh_osal_pinctrl_lookup_state(void *p, const char *name);
void *fh_osal_pinctrl_get_select(void *dev, const char *name);
int fh_osal_pinctrl_pm_select_default_state(void *dev);
int fh_osal_pinctrl_select_state(void *p, void *s);
int fh_osal_clk_prepare_enable(void *clk);
int fh_osal_clk_enable(void *clk);
void fh_osal_clk_disable(void *clk);
void fh_osal_clk_disable_unprepare(void *clk);
unsigned long fh_osal_clk_get_rate(void *clk);
int fh_osal_clk_set_rate(void *clk, unsigned long rate);
int fh_osal_clk_prepare(void * clk);
void fh_osal_clk_unprepare(void *clk);
void *fh_osal_clk_get(void *dev, const char *id);
void fh_osal_clk_put(void *clk);
int fh_osal_clk_set_parent(void *clk, void *parent);
void *fh_osal_clk_get_parent(void *clk);
bool fh_osal_clk_is_enabled(void *clk);
#endif
