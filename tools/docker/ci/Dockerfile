FROM build AS sdk
ENV Boost_ROOT=/opt/boost
ENV otcommon_ROOT=/opt/otcommon
ENV CsLibGuarded_ROOT=/opt/libguarded
COPY --from=boost /opt/boost /opt/boost
COPY --from=cmake /opt/cmake /opt/cmake
COPY --from=iwyu /opt/iwyu /usr
COPY --from=otcommon /opt/otcommon /opt/otcommon
COPY --from=libguarded /opt/libguarded /opt/libguarded
COPY --from=files build-opentxs.sh /usr/bin
COPY --from=files check-formatting.sh /usr/bin
COPY --from=files run-iwyu.sh /usr/bin
RUN chmod a+x /usr/bin/build-opentxs.sh
RUN chmod a+x /usr/bin/check-formatting.sh
RUN chmod a+x /usr/bin/run-iwyu.sh
RUN ln -s /usr/bin/python3 /usr/bin/python
ENTRYPOINT [ "/usr/bin/build-opentxs.sh" ]
CMD []
