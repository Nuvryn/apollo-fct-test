#include <stdint.h>
typedef struct
{
    volatile uint32_t CR, ISR, OPFCCR, OOR, OMAR, NLR, OCOLR, FGPFCCR, FGOR, FGMAR;
} test_dma_regs_t;
extern test_dma_regs_t test_regs;
#define DMA2D (&test_regs)
#define DMA2D_CR_MODE_Pos 16U
#define DMA2D_NLR_PL_Pos 16U
#define DMA2D_CR_START 1U
#define DMA2D_ISR_TEIF 1U
#define DMA2D_ISR_TCIF 2U
#define DMA2D_ISR_CEIF 32U
void test_dma_reset(void);
#define __HAL_RCC_DMA2D_CLK_ENABLE() ((void) 0)
#define __HAL_RCC_DMA2D_FORCE_RESET() test_dma_reset()
#define __HAL_RCC_DMA2D_RELEASE_RESET() ((void) 0)
#define __DSB() ((void) 0)
