.PHONY: run rgdb rtrace clean
a.out:
	gcc -finstrument-functions triangle_headless.c $$(pkg-config --cflags gbm) $$(pkg-config --libs-only-L gbm) -lgbm -lEGL -lGL

run:
	mesa_glthread=false GALLIUM_THREAD=0 AMD_DEBUG=nodma,nofmask,nodccmsaa,nodccstore,nodcc,noexporteddcc,nodisplaydcc,nodisplaytiling,notiling,no2d,nohyperz,nodpbb,nooutoforder,nonggc,nongg,noefc,nodmashaders,nofastdlist,nowcstream,nowc,nooptvariant ./a.out

rgdb:
	mesa_glthread=false GALLIUM_THREAD=0 AMD_DEBUG=nodma,nofmask,nodccmsaa,nodccstore,nodcc,noexporteddcc,nodisplaydcc,nodisplaytiling,notiling,no2d,nohyperz,nodpbb,nooutoforder,nonggc,nongg,noefc,nodmashaders,nofastdlist,nowcstream,nowc,nooptvariant gdb ./a.out

rtrace:
	mesa_glthread=false GALLIUM_THREAD=0 AMD_DEBUG=nodma,nofmask,nodccmsaa,nodccstore,nodcc,noexporteddcc,nodisplaydcc,nodisplaytiling,notiling,no2d,nohyperz,nodpbb,nooutoforder,nonggc,nongg,noefc,nodmashaders,nofastdlist,nowcstream,nowc,nooptvariant uftrace record -a ./a.out

rplay:
	uftrace replay

clean:
	rm -rf a.out
	rm -rf uftrace*
	rm -rf triangle.ppm
