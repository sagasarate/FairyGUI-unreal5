基于[liaowangdezhuzhu](https://github.com/liaowangdezhuzhu/FairyGUI-unreal5)的unreal5版本的魔改版本，做了大量的修改，脱离了SlateUI，使用RHI重写了整个渲染部分，SDisplayObject不再继承SWidget，从而实现了以下功能：  
  
完整合批渲染，对于背包这种情形有比较好的合批优化  
支持了变灰效果  
支持了遮罩  
和编辑器尽量一致的字体渲染效果  
支持了UE的SDF/MSDF，只需要在字体设置里开启距离场渲染就可启用，如果你在编辑器里也开启了TextMeshPro，为了保持描边效果一致，需要设置FUIConfig::OutlineSizeIsSDFOffset=true  
优化的tab焦点导航机制，可以限制或者允许tab导航跨Component  


