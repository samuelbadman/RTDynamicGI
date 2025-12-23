if not exist %1Binary mkdir %1Binary

dxc.exe RayGen.hlsl -T lib_6_3 -Fo Binary\RayGen.dxil
dxc.exe ClosestHit.hlsl -T lib_6_3 -Fo Binary\ClosestHit.dxil
dxc.exe Miss.hlsl -T lib_6_3 -Fo Binary\Miss.dxil
pause