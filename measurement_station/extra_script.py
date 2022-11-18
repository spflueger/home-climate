Import("env")

env.AddCustomTarget(
    "Write Device Id to EEPROM",
    dependencies=None,
    actions=["./writeDeviceID.sh $UPLOAD_FLAGS"],
    title="Write Device Id to EEPROM",
    description=None,
    always_build=True
)