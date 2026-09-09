# Firmware fixture corpus

The behaviour suites use these existing fixtures, independent of the empty or
growing production corpus. Question wording and IDs are unchanged. The QDB4
migration removes deck metadata and substitutes the precise sexual flag in the
test fixtures; production submissions require human reclassification.

`just fw-test` builds QDB4 fixtures and the separate shipped corpora. Guards
verify that English has more than 20 default-permitted questions, includes depth
3 and both content flags, and every question fits the configured byte limit.
Metadata-only protocol fixtures exercise every overlapping permission combination,
singletons, empty pools, recency relaxation, and the depth-3 breather.
